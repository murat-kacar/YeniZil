#pragma once

#include <Arduino.h>
#include <cstdint>
#include "../kernel/component.h"

namespace yenizil {

class PowerManager : public Component {                   // İşlemci güç ayarları
 public:
  explicit PowerManager(uint32_t cpuMhz) : cpuMhz_(cpuMhz) {}

  void begin() override { static_cast<void>(setCpuFrequencyMhz(cpuMhz_)); }  // Ayarlanamazsa işlemci varsayılan frekansta kalır: sadece tüketim artar
  void update(uint64_t) override {}

 private:
  uint32_t cpuMhz_;                                       // İşlemci frekansı (MHz)
};

}  // namespace yenizil
