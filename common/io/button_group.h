#pragma once

#include <array>
#include <cstddef>
#include "../config/input_config.h"
#include "../kernel/polling_component.h"
#include "digital_pin.h"
#include "press_detector.h"

struct ButtonPin {                                        // Kimlikli buton bağlantısı
  uint8_t id;                                             // Olayla gelen kimlik, ör. daire numarası
  uint8_t pin;                                            // Buton pini
};

template <std::size_t N>
class ButtonGroup : public PollingComponent {             // Kimlikli N buton, olay kimlikle gelir
 public:
  using Handler = void (*)(uint8_t id);                   // Basış olayı işleyicisi

  ButtonGroup(const ButtonPin (&buttons)[N], uint8_t activeLevel, const PressConfig& press)
      : PollingComponent(config::kSamplePeriodMs), buttons_(buttons), activeLevel_(activeLevel), press_(press) {}

  void onPress(Handler handler) { handler_ = handler; }   // Geçerli basışta çağrılacak işleyiciyi bağlar

  void begin() override {
    for (const ButtonPin& button : buttons_) setupInput(button.pin, activeLevel_);
  }

 protected:
  void poll(uint64_t nowMs) override {
    for (std::size_t i = 0; i < N; ++i) {
      const bool pressed = isActive(buttons_[i].pin, activeLevel_);
      if (detectors_[i].update(pressed, nowMs, press_) && handler_ != nullptr) handler_(buttons_[i].id);
    }
  }

 private:
  const ButtonPin (&buttons_)[N];                         // Buton tablosu, hardware.h içinde
  uint8_t                      activeLevel_;              // Basılıyken okunan seviye
  PressConfig                  press_;                    // Basış kuralları
  std::array<PressDetector, N> detectors_{};              // Buton başına basış durumu
  Handler                      handler_ = nullptr;        // Basış işleyicisi
};
