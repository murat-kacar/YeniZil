#pragma once

#include <cstdint>

namespace yenizil {

struct PressSettings {                                    // Basış kuralları
  uint32_t minMs;                                         // En kısa geçerli basış (ms), altı parazit
  uint32_t maxMs;                                         // En uzun geçerli basış (ms), üstü kısa devre
  uint32_t cooldownMs;                                    // Kabul edilen basıştan sonra yeni basış için bekleme (ms)
};

}  // namespace yenizil
