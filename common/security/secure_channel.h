#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "../kernel/enum_value.h"
#include "../net/frame.h"
#include "../net/nodes.h"
#include "../net/protocol.h"
#include "ccm_cipher.h"
#include "counter_store.h"
#include "replay_window.h"
#include "tx_counter.h"

namespace yenizil {

class SecureChannel {                                     // Tek ağın çerçevelerini üretir ve doğrular. Sabit sıra: ucuz denetimler → AES-CCM → tekrar penceresi. Tekrar koruması gönderen MAC'e göre (IEEE 802.15.4)
 public:
  static constexpr std::size_t kMaxPeers = 16;            // Bir açılışta izlenebilen en fazla gönderici kart

  SecureChannel(CcmCipher& cipher, CounterStore& store, TxCounter& txCounter, NetworkId networkId)
      : cipher_(cipher), store_(store), txCounter_(txCounter), networkId_(networkId) {}

  [[nodiscard]] bool begin(const MacAddress& mac) {       // Kartın MAC'ini alır, NVS'yi açar ve anahtarı yükler, başarısızsa false
    mac_ = mac;
    return store_.begin() && cipher_.begin();
  }

  [[nodiscard]] std::optional<frame::Bytes> seal(NodeId source, NodeId destination, MessageType type) {  // Yeni sayaçla şifreli ve imzalı çerçeve üretir
    const std::optional<FrameCounter> counter = txCounter_.next();
    if (!counter) return std::nullopt;                    // Sayaç yoksa gönderim yok: nonce tekrarlanmaz
    const frame::Header header{kProtocolVersion, networkId_, mac_, source, destination, *counter};
    frame::Bytes bytes = frame::encodeHeader(header);
    const std::array<uint8_t, frame::kPayloadSize> plain = {toUnderlying(type)};
    if (!cipher_.seal(frame::nonce(header), frame::header(bytes), plain, frame::payload(bytes), frame::tag(bytes))) return std::nullopt;
    return bytes;
  }

  [[nodiscard]] std::optional<Message> open(const frame::Bytes& bytes) {  // Gelen çerçeveyi denetler, geçerli ve yeniyse mesajı döndürür
    const frame::Header header = frame::decodeHeader(bytes);
    if (header.version != kProtocolVersion || header.networkId != networkId_) return std::nullopt;  // Başka sürüm, karttaki öteki ağ ya da komşu apartman
    if (header.sourceMac == mac_) return std::nullopt;    // Kendi yankısı
    if (!isNode(header.source) || !isDestination(header.destination)) return std::nullopt;  // Olmayan ünite
    if (const Peer* known = findPeer(header.sourceMac); known != nullptr && !known->window.isFresh(toUnderlying(header.counter))) return std::nullopt;  // Aynı mesajın başka kopyası: şifre çözmeden atılır
    std::array<uint8_t, frame::kPayloadSize> plain{};
    if (!cipher_.open(frame::nonce(header), frame::header(bytes), frame::payload(bytes), frame::tag(bytes), plain)) return std::nullopt;  // Sahte ya da bozulmuş
    Peer* peer = peerFor(header.sourceMac);               // Yer sadece doğrulanmış göndericiye ayrılır: sahte MAC'ler tabloyu dolduramaz
    if (peer == nullptr || !peer->window.isFresh(toUnderlying(header.counter))) return std::nullopt;  // Tablo dolu ya da NVS'deki kayda göre eski
    peer->window.markSeen(toUnderlying(header.counter));                // Pencere sadece doğrulanmış çerçeveyle ilerler: sahte yüksek sayaç gerçek mesajları engelleyemez
    if (!isKnownMessageType(plain[0])) return std::nullopt;
    return Message{header.sourceMac, header.source, header.destination, header.counter, static_cast<MessageType>(plain[0])};
  }

  [[nodiscard]] bool commit(const Message& message) {     // Eylemden önce çağrılır: sayacı kalıcı kaydeder, yeniden başlamadan sonra aynı çerçeve kabul edilmez
    return store_.saveRxCounter(message.sourceMac, toUnderlying(message.counter));
  }

 private:
  struct Peer {                                           // İzlenen gönderici kart
    bool         used = false;                            // Yuva dolu mu
    MacAddress   mac{};                                   // Kartın MAC adresi
    ReplayWindow window;                                  // Tekrar penceresi
  };

  Peer* findPeer(const MacAddress& mac) {                 // Göndericinin yuvası, yoksa nullptr
    const auto found = std::find_if(peers_.begin(), peers_.end(), [&mac](const Peer& peer) { return peer.used && peer.mac == mac; });
    return found != peers_.end() ? &*found : nullptr;
  }

  Peer* peerFor(const MacAddress& mac) {                  // Göndericinin yuvası; ilk görülüşte yer ayrılır ve kayıtlı sayaç NVS'den yüklenir
    if (Peer* known = findPeer(mac)) return known;
    const auto slot = std::find_if(peers_.begin(), peers_.end(), [](const Peer& peer) { return !peer.used; });
    if (slot == peers_.end()) return nullptr;
    slot->used = true;
    slot->mac  = mac;
    if (const std::optional<uint32_t> saved = store_.loadRxCounter(mac)) slot->window.restore(*saved);
    return &*slot;
  }

  CcmCipher&                    cipher_;                  // Bu ağın AES-CCM'i
  CounterStore&                 store_;                   // Alıcı sayaçlarının kalıcı kaydı
  TxCounter&                    txCounter_;               // Kartın gönderme sayacı, ağlar arasında ortak
  NetworkId                     networkId_;               // Ağ kimliği
  MacAddress                    mac_{};                   // Bu kartın MAC adresi
  std::array<Peer, kMaxPeers>   peers_{};                 // Gönderici başına tekrar penceresi
};

}  // namespace yenizil
