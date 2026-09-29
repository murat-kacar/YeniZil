#pragma once

#include <cstdint>
#include "../kernel/clock.h"
#include "../kernel/component.h"
#include "digital_pin.h"

namespace yenizil {

class PulseOutput : public Component {                    // Belirli süre aktif kalan çıkış
 public:
  PulseOutput(uint8_t pin, uint8_t activeLevel, uint32_t durationMs)
      : pin_(pin), activeLevel_(activeLevel), durationMs_(durationMs) {}

  void begin() override { setupOutput(pin_, activeLevel_); }

  void activate() {                                       // Çıkışı süre boyunca aktif yapar, aktifken gelen çağrı yok sayılır
    if (active_) return;
    active_  = true;
    offAtMs_ = monotonicMs() + durationMs_;
    writeOutput(pin_, activeLevel_, true);
  }

  void update(uint64_t nowMs) override {
    if (!active_ || nowMs < offAtMs_) return;
    active_ = false;
    writeOutput(pin_, activeLevel_, false);
  }

  uint64_t nextDeadlineMs() const override { return active_ ? offAtMs_ : kNoDeadlineMs; }

 private:
  uint8_t  pin_;                                          // Çıkış pini
  uint8_t  activeLevel_;                                  // Aktif seviye
  uint32_t durationMs_;                                   // Aktif kalma süresi (ms)
  bool     active_  = false;                              // Şu an aktif mi
  uint64_t offAtMs_ = 0;                                  // Pasife döneceği an
};

}  // namespace yenizil
