#pragma once

#include <cstdint>
#include "../kernel/clock.h"
#include "../kernel/component.h"
#include "digital_pin.h"

namespace yenizil {

class IndicatorLed : public Component {                   // Gösterge LED'i: sürekli yanar ya da yanıp söner
 public:
  IndicatorLed(uint8_t pin, uint8_t activeLevel, uint32_t blinkHalfPeriodMs)
      : pin_(pin), activeLevel_(activeLevel), blinkHalfPeriodMs_(blinkHalfPeriodMs) {}

  void begin() override {
    setupOutput(pin_, activeLevel_);
    writeOutput(pin_, activeLevel_, lit_);                // begin'den önce seçilmiş durum da uygulanır
  }

  void turnOn() {                                         // Sürekli yanar
    blinking_ = false;
    set(true);
  }

  void blink() {                                          // Yanıp söner, hemen yanarak başlar
    if (blinking_) return;
    blinking_     = true;
    nextToggleMs_ = monotonicMs() + blinkHalfPeriodMs_;
    set(true);
  }

  void update(uint64_t nowMs) override {                  // Yanıp sönme zamanı geldiyse durumu değiştirir
    if (!blinking_ || nowMs < nextToggleMs_) return;
    nextToggleMs_ = nowMs + blinkHalfPeriodMs_;
    set(!lit_);
  }

  uint64_t nextDeadlineMs() const override { return blinking_ ? nextToggleMs_ : kNoDeadlineMs; }

 private:
  void set(bool lit) {                                    // Durum değiştiyse pini yazar
    if (lit == lit_) return;
    lit_ = lit;
    writeOutput(pin_, activeLevel_, lit_);
  }

  uint8_t  pin_;                                          // LED pini
  uint8_t  activeLevel_;                                  // LED'i yakan seviye
  uint32_t blinkHalfPeriodMs_;                            // Yanık ya da sönük kalma süresi (ms)
  bool     blinking_     = false;                         // Yanıp sönüyor mu
  bool     lit_          = false;                         // Şu an yanık mı
  uint64_t nextToggleMs_ = 0;                             // Sıradaki değişimin anı
};

}  // namespace yenizil
