#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "../net/frame.h"
#include "../net/nodes.h"
#include "../net/protocol.h"
#include "ccm_cipher.h"
#include "counter_store.h"
#include "replay_window.h"

namespace yenizil {

class SecureChannel {                                     // Çerçeve üretir ve doğrular. Sabit sıra: ucuz denetimler → AES-CCM → tekrar penceresi. Tekrar koruması gönderen MAC'e göre (IEEE 802.15.4)
 public:
  static constexpr uint32_t    kCounterReserve = 1000;    // Sayaç rezervi: OpenThread STORE_FRAME_COUNTER_AHEAD varsayılanı
  static constexpr std::size_t kMaxPeers       = 16;      // Bir açılışta izlenebilen en fazla gönderici kart

  SecureChannel(CcmCipher& cipher, CounterStore& store, uint32_t apartmentId) : cipher_(cipher), store_(store), apartmentId_(apartmentId) {}

  [[nodiscard]] bool begin(const MacAddress& mac) {       // Kartın MAC'ini alır, anahtarı ve gönderme sayacını yükler, başarısızsa false
    mac_ = mac;
    if (!store_.begin() || !cipher_.begin()) return false;
    nextCounter_  = store_.loadTxReserve();               // Yeniden başlamada rezervin sonundan devam: kullanılmış sayaç tekrar kullanılmaz
    reservedUpTo_ = nextCounter_;
    return true;
  }

  [[nodiscard]] std::optional<frame::Bytes> seal(NodeId source, NodeId destination, MessageType type) {  // Yeni sayaçla şifreli ve imzalı çerçeve üretir
    if (!reserveCounter()) return std::nullopt;
    const frame::Header header{kProtocolVersion, apartmentId_, mac_, source, destination, nextCounter_++};
    frame::Bytes bytes = frame::encodeHeader(header);
    const std::array<uint8_t, frame::kPayloadSize> plain = {static_cast<uint8_t>(type)};
    if (!cipher_.seal(frame::nonce(header), frame::header(bytes), plain, frame::payload(bytes), frame::tag(bytes))) return std::nullopt;
    return bytes;
  }

  [[nodiscard]] std::optional<Message> open(const frame::Bytes& bytes) {  // Gelen çerçeveyi denetler, geçerli ve yeniyse mesajı döndürür
    const frame::Header header = frame::decodeHeader(bytes);
    if (header.version != kProtocolVersion || header.apartmentId != apartmentId_) return std::nullopt;  // Başka sürüm ya da komşu apartman
    if (header.sourceMac == mac_) return std::nullopt;    // Kendi yankısı
    if (header.source >= kNodeCount || !isDestination(header.destination)) return std::nullopt;  // Olmayan ünite
    if (const Peer* known = findPeer(header.sourceMac); known != nullptr && !known->window.isFresh(header.counter)) return std::nullopt;  // Aynı mesajın başka kopyası: şifre çözmeden atılır
    std::array<uint8_t, frame::kPayloadSize> plain{};
    if (!cipher_.open(frame::nonce(header), frame::header(bytes), frame::payload(bytes), frame::tag(bytes), plain)) return std::nullopt;  // Sahte ya da bozulmuş
    Peer* peer = peerFor(header.sourceMac);               // Yer sadece doğrulanmış göndericiye ayrılır: sahte MAC'ler tabloyu dolduramaz
    if (peer == nullptr || !peer->window.isFresh(header.counter)) return std::nullopt;  // Tablo dolu ya da NVS'deki kayda göre eski
    peer->window.markSeen(header.counter);                // Pencere sadece doğrulanmış çerçeveyle ilerler: sahte yüksek sayaç gerçek mesajları engelleyemez
    if (!isKnownMessageType(plain[0])) return std::nullopt;
    return Message{header.sourceMac, header.source, header.destination, header.counter, static_cast<MessageType>(plain[0])};
  }

  [[nodiscard]] bool commit(const Message& message) {     // Eylemden önce çağrılır: sayacı kalıcı kaydeder, yeniden başlamadan sonra aynı çerçeve kabul edilmez
    return store_.saveRxCounter(message.sourceMac, message.counter);
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

  bool reserveCounter() {                                 // Sıradaki sayaç flash'taki rezervin içinde mi, değilse yeni rezerv kaydeder
    if (nextCounter_ < reservedUpTo_) return true;
    if (nextCounter_ > UINT32_MAX - kCounterReserve) return false;  // Sayaç tükendi: nonce tekrarı yerine gönderim durur, anahtar değiştirilmeli
    if (!store_.saveTxReserve(nextCounter_ + kCounterReserve)) return false;  // Rezerv kaydedilemezse yeniden başlamada sayaç tekrar kullanılabilirdi
    reservedUpTo_ = nextCounter_ + kCounterReserve;
    return true;
  }

  CcmCipher&                    cipher_;                  // AES-CCM
  CounterStore&                 store_;                   // Sayaç kalıcı kaydı
  uint32_t                      apartmentId_;             // Ağ (apartman) kimliği
  MacAddress                    mac_{};                   // Bu kartın MAC adresi
  uint32_t                      nextCounter_  = 0;        // Sıradaki gönderme sayacı
  uint32_t                      reservedUpTo_ = 0;        // Flash'a kaydedilmiş rezervin sonu
  std::array<Peer, kMaxPeers>   peers_{};                 // Gönderici başına tekrar penceresi
};

}  // namespace yenizil
