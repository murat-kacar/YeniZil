#pragma once

#include "../config/input_config.h"
#include "../kernel/polling_component.h"
#include "digital_pin.h"
#include "press_detector.h"

class Button : public PollingComponent {                  // Tek buton
 public:
  using Handler = void (*)();                             // Buton olayı işleyicisi

  Button(uint8_t pin, uint8_t activeLevel, const PressConfig& press)
      : PollingComponent(config::kSamplePeriodMs), pin_(pin), activeLevel_(activeLevel), press_(press) {}

  void onPress(Handler handler) { pressHandler_ = handler; }          // Geçerli basışta çağrılır
  void onStartupHold(Handler handler) { startupHandler_ = handler; }  // Açılışta basılı tutulup bırakılınca çağrılır
  void begin() override { setupInput(pin_, activeLevel_); }

 protected:
  void poll(uint64_t nowMs) override {
    switch (detector_.update(isActive(pin_, activeLevel_), nowMs, press_)) {
      case PressEvent::kPress:       notify(pressHandler_); break;
      case PressEvent::kStartupHold: notify(startupHandler_); break;
      case PressEvent::kNone:        break;
    }
  }

 private:
  static void notify(Handler handler) {                   // İşleyici bağlıysa çağırır
    if (handler != nullptr) handler();
  }

  uint8_t       pin_;                                     // Buton pini
  uint8_t       activeLevel_;                             // Basılıyken okunan seviye
  PressConfig   press_;                                   // Basış kuralları
  PressDetector detector_;                                // Basış durumu
  Handler       pressHandler_   = nullptr;                // Basış işleyicisi
  Handler       startupHandler_ = nullptr;                // Açılışta basılı tutma işleyicisi
};
