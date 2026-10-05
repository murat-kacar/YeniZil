#pragma once

#include <cstdint>
#include <optional>
#include "../kernel/component.h"
#include "../net/protocol.h"
#include "counter_store.h"

namespace yenizil {

class TxCounter : public Component {                      // Kartın gönderme sayacı: karttaki tüm ağlar paylaşır, nonce (MAC + sayaç) hiçbir anahtarla tekrarlanmaz
 public:
  static constexpr uint32_t kReserve = 1000;              // Sayaç rezervi: OpenThread STORE_FRAME_COUNTER_AHEAD varsayılanı

  explicit TxCounter(CounterStore& store) : store_(store) {}

  void begin() override {                                 // Kayıtlı rezervi yükler, NVS açılamazsa sayaç kapalı kalır
    ready_ = store_.begin();
    if (ready_) next_ = reservedUpTo_ = store_.loadTxReserve();  // Yeniden başlamada rezervin sonundan devam: kullanılmış sayaç tekrar kullanılmaz
  }

  void update(uint64_t) override {}

  [[nodiscard]] std::optional<FrameCounter> next() {      // Sıradaki sayaç, kapalıysa ya da rezerv kaydedilemezse boş
    if (!ready_ || !reserve()) return std::nullopt;
    return FrameCounter{next_++};
  }

 private:
  bool reserve() {                                        // Sıradaki sayaç flash'taki rezervin içinde mi, değilse yeni rezerv kaydeder
    if (next_ < reservedUpTo_) return true;
    if (next_ > UINT32_MAX - kReserve) return false;      // Sayaç tükendi: nonce tekrarı yerine gönderim durur, anahtarlar değiştirilmeli
    if (!store_.saveTxReserve(next_ + kReserve)) return false;  // Rezerv kaydedilemezse yeniden başlamada sayaç tekrar kullanılabilirdi
    reservedUpTo_ = next_ + kReserve;
    return true;
  }

  CounterStore& store_;                                   // Sayaç kalıcı kaydı
  bool          ready_        = false;                    // Rezerv yüklendi mi
  uint32_t      next_         = 0;                        // Sıradaki gönderme sayacı
  uint32_t      reservedUpTo_ = 0;                        // Flash'a kaydedilmiş rezervin sonu
};

}  // namespace yenizil
