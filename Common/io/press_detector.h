#pragma once

#include <stdint.h>

struct PressConfig {                                      // Basış kuralları
  uint32_t minMs;                                         // En kısa geçerli basış (ms), altı parazit
  uint32_t maxMs;                                         // En uzun geçerli basış (ms), üstü kısa devre
  uint32_t cooldownMs;                                    // Kabul edilen basıştan sonra yeni basış için bekleme (ms)
};

class PressDetector {                                     // Seviye ve zamandan geçerli basışı çıkarır, karar bırakınca verilir
 public:
  [[nodiscard]] constexpr bool update(bool pressed, uint64_t nowMs, const PressConfig& config) {  // Geçerli bir basış bittiyse true
    if (!armed_) {                                        // Açılışta basılı gelen buton bırakılana kadar yok sayılır
      armed_ = !pressed;
      return false;
    }
    if (pressed == pressed_) return false;
    pressed_ = pressed;
    if (pressed) {
      pressStartMs_ = nowMs;
      return false;
    }
    return accept(nowMs - pressStartMs_, nowMs, config);
  }

 private:
  constexpr bool accept(uint64_t durationMs, uint64_t nowMs, const PressConfig& config) {  // Süre ve bekleme kurallarını uygular
    if (durationMs < config.minMs || durationMs > config.maxMs) return false;
    if (hasAccepted_ && nowMs - lastAcceptMs_ < config.cooldownMs) return false;
    hasAccepted_  = true;
    lastAcceptMs_ = nowMs;
    return true;
  }

  bool     armed_        = false;                         // Açılıştan sonra buton en az bir kez bırakılmış görüldü mü
  bool     pressed_      = false;                         // Son görülen durum
  bool     hasAccepted_  = false;                         // Daha önce kabul edilmiş basış var mı
  uint64_t pressStartMs_ = 0;                             // Basışın başladığı an
  uint64_t lastAcceptMs_ = 0;                             // Son kabul edilen basışın bittiği an
};
