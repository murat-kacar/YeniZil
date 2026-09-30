#pragma once

#include <cstdint>
#include <optional>
#include "../config/press_settings.h"

namespace yenizil {

class PressDetector {                                     // Seviye ve zamandan geçerli basışı çıkarır, karar bırakınca verilir (sonlu durum makinesi)
 public:
  [[nodiscard]] constexpr bool update(bool pressed, uint64_t nowMs, const PressSettings& settings) {  // Geçerli bir basış bittiyse true
    switch (state_) {
      case State::kWaitingForRelease:                     // Açılışta basılı gelen buton bırakılana kadar yok sayılır
        if (!pressed) state_ = State::kReleased;
        return false;
      case State::kReleased:
        if (!pressed) return false;
        state_        = State::kPressed;
        pressStartMs_ = nowMs;
        return false;
      case State::kPressed:
        if (pressed) return false;
        state_ = State::kReleased;
        return accept(nowMs - pressStartMs_, nowMs, settings);
    }
    return false;
  }

 private:
  enum class State : uint8_t {                            // Buton durumu
    kWaitingForRelease,                                   // Açılış: bırakılması bekleniyor
    kReleased,                                            // Bırakılmış
    kPressed,                                             // Basılı
  };

  constexpr bool accept(uint64_t durationMs, uint64_t nowMs, const PressSettings& settings) {  // Süre ve bekleme kurallarını uygular
    if (durationMs < settings.minMs || durationMs > settings.maxMs) return false;
    if (lastAcceptMs_ && nowMs - *lastAcceptMs_ < settings.cooldownMs) return false;
    lastAcceptMs_ = nowMs;
    return true;
  }

  State                   state_        = State::kWaitingForRelease;  // Güncel durum
  uint64_t                pressStartMs_ = 0;              // Basışın başladığı an
  std::optional<uint64_t> lastAcceptMs_;                  // Son kabul edilen basışın bittiği an, hiç yoksa boş
};

}  // namespace yenizil
