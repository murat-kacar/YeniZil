#pragma once

#include <stdint.h>
#include <algorithm>
#include <iterator>

namespace board {                                                     // ESP32-C3 Super Mini

inline constexpr uint8_t kSafeGpios[] = {0, 1, 3, 4, 5, 6, 7, 10};    // Açılışı etkilemeyen pinler: 2/8/9 strapping, 18/19 USB, 20/21 UART0

constexpr bool isSafeGpio(uint8_t pin) {                              // Pin harici bağlantı için güvenli mi
  return std::find(std::begin(kSafeGpios), std::end(kSafeGpios), pin) != std::end(kSafeGpios);
}

}  // namespace board
