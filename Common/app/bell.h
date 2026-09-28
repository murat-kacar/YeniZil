#pragma once

#include "../io/pulse_output.h"

class Bell {                                              // Zil
 public:
  Bell(uint8_t pin, uint8_t activeLevel, uint32_t pulseMs) : output_(pin, activeLevel, pulseMs) {}

  void ring() { output_.activate(); }                     // Zili çalar

 private:
  PulseOutput output_;                                    // Zil tetik çıkışı
};
