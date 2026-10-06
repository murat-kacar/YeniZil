#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>

namespace yenizil::board {                                            // ESP32-C3 Super Mini

inline constexpr uint8_t kSafeGpios[] = {0, 1, 3, 4, 5, 6, 7, 10, 20};  // Açılışı etkilemeyen pinler: 2/8/9 strapping, 18/19 USB, 21 UART0 TX (açılışta ROM sürer). 20 UART0 RX: açılışta giriş, kod seri port kullanmıyor

constexpr bool isSafeGpio(uint8_t pin) {                              // Pin harici bağlantı için güvenli mi
  return std::find(std::begin(kSafeGpios), std::end(kSafeGpios), pin) != std::end(kSafeGpios);
}

}  // namespace yenizil::board
