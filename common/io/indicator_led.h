#pragma once

#include <cstdint>
#include "../kernel/clock.h"
#include "../kernel/component.h"
#include "digital_pin.h"

namespace yenizil {

class IndicatorLed : public Component {                   // Gösterge LED'i: sönük, sürekli yanan ya da yanıp sönen (sonlu durum makinesi)
 public:
  IndicatorLed(uint8_t pin, uint8_t activeLevel, uint32_t blinkHalfPeriodMs)
      : pin_(pin), activeLevel_(activeLevel), blinkHalfPeriodMs_(blinkHalfPeriodMs) {}

  void begin() override {
    setupOutput(pin_, activeLevel_);
    writeOutput(pin_, activeLevel_, lit_);                // begin'den önce seçilmiş durum da uygulanır
  }

  void turnOn() {                                         // Sürekli yanar
    mode_ = Mode::kSteady;
    set(true);
  }

  void blink() {                                          // Yanıp söner, hemen yanarak başlar
    if (mode_ == Mode::kBlinking) return;
    mode_         = Mode::kBlinking;
    nextToggleMs_ = monotonicMs() + blinkHalfPeriodMs_;
    set(true);
  }

  void update(uint64_t nowMs) override {                  // Yanıp sönme zamanı geldiyse durumu değiştirir
    if (mode_ != Mode::kBlinking || nowMs < nextToggleMs_) return;
    nextToggleMs_ = nowMs + blinkHalfPeriodMs_;
    set(!lit_);
  }

  uint64_t nextDeadlineMs() const override { return mode_ == Mode::kBlinking ? nextToggleMs_ : kNoDeadlineMs; }

 private:
  enum class Mode : uint8_t {                             // LED modu
    kOff,                                                 // Sönük, açılış durumu
    kSteady,                                              // Sürekli yanar
    kBlinking,                                            // Yanıp söner
  };

  void set(bool lit) {                                    // Durum değiştiyse pini yazar
    if (lit == lit_) return;
    lit_ = lit;
    writeOutput(pin_, activeLevel_, lit_);
  }

  uint8_t  pin_;                                          // LED pini
  uint8_t  activeLevel_;                                  // LED'i yakan seviye
  uint32_t blinkHalfPeriodMs_;                            // Yanık ya da sönük kalma süresi (ms)
  Mode     mode_         = Mode::kOff;                    // Güncel mod
  bool     lit_          = false;                         // Pinin şu anki durumu, yanıp sönerken değişir
  uint64_t nextToggleMs_ = 0;                             // Sıradaki değişimin anı
};

}  // namespace yenizil
