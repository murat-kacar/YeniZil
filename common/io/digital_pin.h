#pragma once

#include <Arduino.h>
#include <driver/gpio.h>
#include <cstdint>

namespace yenizil {

constexpr uint8_t inactiveLevel(uint8_t activeLevel) { return activeLevel == HIGH ? LOW : HIGH; }  // Aktif seviyenin tersi

inline void setupInput(uint8_t pin, uint8_t activeLevel) {  // Girişi, pasif seviyeye çeken dahili dirençle ayarlar
  pinMode(pin, activeLevel == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
}

inline bool isActive(uint8_t pin, uint8_t activeLevel) {    // Giriş aktif seviyede mi
  return digitalRead(pin) == activeLevel;
}

inline void setupOutput(uint8_t pin, uint8_t activeLevel) {  // Çıkışı pasif seviyede açar, açılışta titreme olmaz
  gpio_set_level(static_cast<gpio_num_t>(pin), inactiveLevel(activeLevel));  // Arduino digitalWrite() pinMode()'dan önce çalışmıyor
  pinMode(pin, OUTPUT);
}

inline void writeOutput(uint8_t pin, uint8_t activeLevel, bool active) {  // Çıkışı aktif ya da pasif yapar
  digitalWrite(pin, active ? activeLevel : inactiveLevel(activeLevel));
}

}  // namespace yenizil
