#pragma once

#include <Arduino.h>
#include "../kernel/component.h"

class PowerManager : public Component {                   // İşlemci güç ayarları
 public:
  explicit PowerManager(uint32_t cpuMhz) : cpuMhz_(cpuMhz) {}

  void begin() override { setCpuFrequencyMhz(cpuMhz_); }
  void update(uint64_t) override {}

 private:
  uint32_t cpuMhz_;                                       // İşlemci frekansı (MHz)
};
