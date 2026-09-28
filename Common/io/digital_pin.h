#pragma once

#include <Arduino.h>

inline void setupInput(uint8_t pin, uint8_t activeLevel) {  // Girişi, pasif seviyeye çeken dahili dirençle ayarlar
  pinMode(pin, activeLevel == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
}

inline bool isActive(uint8_t pin, uint8_t activeLevel) {    // Giriş aktif seviyede mi
  return digitalRead(pin) == activeLevel;
}
