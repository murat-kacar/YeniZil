#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "../config/input_config.h"
#include "../config/press_settings.h"
#include "../kernel/callback.h"
#include "../kernel/polling_component.h"
#include "button_pin.h"
#include "digital_pin.h"
#include "press_detector.h"

namespace yenizil {

template <typename Id, std::size_t N>
class ButtonGroup : public PollingComponent {             // Kimlikli N buton, olay kimlikle gelir
 public:
  ButtonGroup(const ButtonPin<Id> (&buttons)[N], uint8_t activeLevel, const PressSettings& press)
      : PollingComponent(config::kSamplePeriodMs), buttons_(buttons), activeLevel_(activeLevel), press_(press) {}

  void onPress(Handler<Id> handler) { handler_ = handler; }  // Geçerli basışta, butonun kimliğiyle çağrılır

  void begin() override {
    for (const ButtonPin<Id>& button : buttons_) setupInput(button.pin, activeLevel_);
  }

 protected:
  void poll(uint64_t nowMs) override {
    for (std::size_t i = 0; i < N; ++i)
      if (detectors_[i].update(isActive(buttons_[i].pin, activeLevel_), nowMs, press_)) callIfSet(handler_, buttons_[i].id);
  }

 private:
  const ButtonPin<Id> (&buttons_)[N];                     // Buton tablosu, hardware.h içinde
  uint8_t                      activeLevel_;              // Basılıyken okunan seviye
  PressSettings                press_;                    // Basış kuralları
  std::array<PressDetector, N> detectors_{};              // Buton başına basış durumu
  Handler<Id>                  handler_ = nullptr;        // Basış işleyicisi
};

}  // namespace yenizil
