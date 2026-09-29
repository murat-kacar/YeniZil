#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <optional>
#include "../config/site_config.h"
#include "../net/frame.h"
#include "../net/nodes.h"
#include "../net/protocol.h"
#include "ccm_cipher.h"
#include "counter_store.h"
#include "replay_window.h"

static_assert(std::any_of(std::begin(site::kApartmentKey), std::end(site::kApartmentKey), [](uint8_t byte) { return byte != 0; }),
              "Apartman anahtarı atanmamış: site_config.h içinde kApartmentKey rastgele doldurulmalı");

class SecureChannel {                                     // Çerçeve üretir ve doğrular. Sabit sıra: ucuz denetimler → AES-CCM → tekrar penceresi → kalıcı kayıt
 public:
  static constexpr uint32_t kCounterReserve = 1000;       // Sayaç rezervi: OpenThread STORE_FRAME_COUNTER_AHEAD varsayılanı

  SecureChannel(CcmCipher& cipher, CounterStore& store) : cipher_(cipher), store_(store) {}

  [[nodiscard]] bool begin(NodeId self) {                 // Anahtarı ve sayaçları yükler, başarısızsa false
    self_ = self;
    if (!store_.begin() || !cipher_.begin()) return false;
    nextCounter_  = store_.loadTxReserve();               // Yeniden başlamada rezervin sonundan devam: kullanılmış sayaç tekrar kullanılmaz
    reservedUpTo_ = nextCounter_;
    for (NodeId source = 0; source < kNodeCount; ++source)
      if (const std::optional<uint32_t> saved = store_.loadRxCounter(source)) windows_[source].restore(*saved);
    return true;
  }

  [[nodiscard]] std::optional<frame::Bytes> seal(NodeId destination, MessageType type) {  // Yeni sayaçla şifreli ve imzalı çerçeve üretir
    if (!reserveCounter()) return std::nullopt;
    const frame::Header header{kProtocolVersion, site::kApartmentId, self_, destination, nextCounter_++};
    frame::Bytes bytes = frame::encodeHeader(header);
    const std::array<uint8_t, frame::kPayloadSize> plain = {static_cast<uint8_t>(type)};
    if (!cipher_.seal(frame::nonce(header), frame::header(bytes), plain, frame::payload(bytes), frame::tag(bytes))) return std::nullopt;
    return bytes;
  }

  [[nodiscard]] std::optional<Message> open(const frame::Bytes& bytes) {  // Gelen çerçeveyi denetler, geçerli ve yeniyse mesajı döndürür
    const frame::Header header = frame::decodeHeader(bytes);
    if (header.version != kProtocolVersion || header.apartmentId != site::kApartmentId) return std::nullopt;  // Başka sürüm ya da komşu apartman
    if (header.source == self_ || header.source >= kNodeCount || header.destination >= kNodeCount) return std::nullopt;  // Kendi yankısı ya da tabloda olmayan ünite
    ReplayWindow& window = windows_[header.source];
    if (!window.isFresh(header.counter)) return std::nullopt;  // Aynı mesajın başka kopyası: şifre çözmeden atılır, pencere değişmez
    std::array<uint8_t, frame::kPayloadSize> plain{};
    if (!cipher_.open(frame::nonce(header), frame::header(bytes), frame::payload(bytes), frame::tag(bytes), plain)) return std::nullopt;  // Sahte ya da bozulmuş
    window.markSeen(header.counter);                      // Pencere sadece doğrulanmış çerçeveyle ilerler: sahte yüksek sayaç gerçek mesajları engelleyemez
    if (!isKnownMessageType(plain[0])) return std::nullopt;
    if (header.destination == self_ && !store_.saveRxCounter(header.source, header.counter)) return std::nullopt;  // Eylemden önce kalıcı kayıt: kaydedilemezse eylem yok
    return Message{header.source, header.destination, header.counter, static_cast<MessageType>(plain[0])};
  }

 private:
  bool reserveCounter() {                                 // Sıradaki sayaç flash'taki rezervin içinde mi, değilse yeni rezerv kaydeder
    if (nextCounter_ < reservedUpTo_) return true;
    if (nextCounter_ > UINT32_MAX - kCounterReserve) return false;  // Sayaç tükendi: nonce tekrarı yerine gönderim durur, anahtar değiştirilmeli
    if (!store_.saveTxReserve(nextCounter_ + kCounterReserve)) return false;  // Rezerv kaydedilemezse yeniden başlamada sayaç tekrar kullanılabilirdi
    reservedUpTo_ = nextCounter_ + kCounterReserve;
    return true;
  }

  CcmCipher&                             cipher_;         // AES-CCM
  CounterStore&                          store_;          // Sayaç kalıcı kaydı
  NodeId                                 self_         = 0;  // Bu ünitenin kimliği
  uint32_t                               nextCounter_  = 0;  // Sıradaki gönderme sayacı
  uint32_t                               reservedUpTo_ = 0;  // Flash'a kaydedilmiş rezervin sonu
  std::array<ReplayWindow, kNodeCount>   windows_{};      // Kaynak başına tekrar penceresi
};
