#pragma once

#include "../config/input_config.h"
#include "../kernel/polling_component.h"
#include "digital_pin.h"
#include "press_detector.h"

class Button : public PollingComponent {                  // Tek buton
 public:
  using Handler = void (*)();                             // Basış olayı işleyicisi

  Button(uint8_t pin, uint8_t activeLevel, const PressConfig& press)
      : PollingComponent(config::kSamplePeriodMs), pin_(pin), activeLevel_(activeLevel), press_(press) {}

  void onPress(Handler handler) { handler_ = handler; }   // Geçerli basışta çağrılacak işleyiciyi bağlar
  void begin() override { setupInput(pin_, activeLevel_); }

 protected:
  void poll(uint64_t nowMs) override {
    if (detector_.update(isActive(pin_, activeLevel_), nowMs, press_) && handler_ != nullptr) handler_();
  }

 private:
  uint8_t       pin_;                                     // Buton pini
  uint8_t       activeLevel_;                             // Basılıyken okunan seviye
  PressConfig   press_;                                   // Basış kuralları
  PressDetector detector_;                                // Basış durumu
  Handler       handler_ = nullptr;                       // Basış işleyicisi
};
