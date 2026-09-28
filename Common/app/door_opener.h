#pragma once

#include "../io/pulse_output.h"

class DoorOpener {                                        // Kapı açıcı
 public:
  DoorOpener(uint8_t pin, uint8_t activeLevel, uint32_t pulseMs) : output_(pin, activeLevel, pulseMs) {}

  void open() { output_.activate(); }                     // Kapıyı açar

 private:
  PulseOutput output_;                                    // Kapı rölesi tetik çıkışı
};
