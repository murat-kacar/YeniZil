#pragma once

#include <array>
#include <optional>
#include "../config/site_config.h"
#include "../net/frame.h"
#include "../net/nodes.h"
#include "../net/protocol.h"
#include "counter_store.h"
#include "replay_window.h"

class SecureChannel {                                     // Çerçeve üretir ve doğrular. Sabit sıra: başlık → doğrulama (Aşama 3) → tekrar denetimi
 public:
  static constexpr uint32_t kCounterReserve = 1000;       // Sayaç rezervi: OpenThread STORE_FRAME_COUNTER_AHEAD varsayılanı

  explicit SecureChannel(CounterStore& store) : store_(store) {}

  void begin(NodeId self) {                               // Kimliği alır, gönderme sayacını flash'tan yükler
    self_ = self;
    store_.begin();
    nextCounter_  = store_.loadTxReserve();               // Yeniden başlamada rezervin sonundan devam: kullanılmış sayaç tekrar kullanılmaz
    reservedUpTo_ = nextCounter_;
  }

  frame::Bytes seal(NodeId destination, MessageType type) {  // Yeni sayaçla giden çerçeveyi üretir
    if (nextCounter_ >= reservedUpTo_) {                  // Rezerv bitti: sayaç kullanılmadan önce yeni rezerv kaydedilir
      reservedUpTo_ = nextCounter_ + kCounterReserve;
      store_.saveTxReserve(reservedUpTo_);
    }
    const frame::Header header{kProtocolVersion, site::kApartmentId, self_, destination, nextCounter_++};
    return frame::encode(header, static_cast<uint8_t>(type));  // Aşama 3: tip şifrelenecek, etiket eklenecek
  }

  [[nodiscard]] std::optional<Message> open(const frame::Bytes& bytes) {  // Gelen çerçeveyi denetler, geçerli ve yeniyse mesajı döndürür
    const frame::Header header = frame::decodeHeader(bytes);
    if (header.version != kProtocolVersion || header.apartmentId != site::kApartmentId) return std::nullopt;  // Başka sürüm ya da komşu apartman
    if (header.source == self_ || header.source >= kNodeCount || header.destination >= kNodeCount) return std::nullopt;  // Kendi yankısı ya da tabloda olmayan ünite
    const uint8_t type = frame::typeByte(bytes);          // Aşama 3: önce AES-CCM ile doğrulanıp çözülecek
    if (!isKnownMessageType(type)) return std::nullopt;
    ReplayWindow& window = windows_[header.source];
    if (!window.isFresh(header.counter)) return std::nullopt;  // Aynı mesajın başka kopyası ya da tekrar
    window.markSeen(header.counter);
    return Message{header.source, header.destination, header.counter, static_cast<MessageType>(type)};
  }

 private:
  CounterStore&                          store_;          // Sayaç kalıcı kaydı
  NodeId                                 self_         = 0;  // Bu ünitenin kimliği
  uint32_t                               nextCounter_  = 0;  // Sıradaki gönderme sayacı
  uint32_t                               reservedUpTo_ = 0;  // Flash'a kaydedilmiş rezervin sonu
  std::array<ReplayWindow, kNodeCount>   windows_{};      // Kaynak başına tekrar penceresi
};
