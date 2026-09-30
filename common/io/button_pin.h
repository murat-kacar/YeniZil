#pragma once

#include <cstdint>

namespace yenizil {

template <typename Id>
struct ButtonPin {                                        // Kimlikli buton bağlantısı: kimlik tipi güçlü tip olabilir, pinle karışmaz
  Id      id;                                             // Olayla gelen kimlik, ör. daire numarası
  uint8_t pin;                                            // Buton pini
};

}  // namespace yenizil
