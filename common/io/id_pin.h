#pragma once

#include <cstdint>

namespace yenizil {

template <typename Id>
struct IdPin {                                            // Kimlikli pin bağlantısı (buton, LED): kimlik tipi güçlü tip olabilir, pinle karışmaz
  Id      id;                                             // Pinin kimliği, ör. daire numarası
  uint8_t pin;                                            // Pin
};

}  // namespace yenizil
