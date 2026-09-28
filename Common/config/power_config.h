#pragma once

#include <stdint.h>

namespace config {                                        // Ürün ayarları

inline constexpr uint32_t kCpuMhz = 80;                   // İşlemci frekansı (MHz): Wi-Fi'ın çalıştığı en düşük değer

static_assert(kCpuMhz == 80 || kCpuMhz == 160, "ESP32-C3'te Wi-Fi ile sadece 80 veya 160 MHz");

}  // namespace config
