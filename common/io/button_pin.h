#pragma once

#include <cstdint>

namespace yenizil {

struct ButtonPin {                                        // Kimlikli buton bağlantısı
  uint8_t id;                                             // Olayla gelen kimlik, ör. daire numarası
  uint8_t pin;                                            // Buton pini
};

}  // namespace yenizil
