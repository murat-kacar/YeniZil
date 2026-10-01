#pragma once

#include <cstdint>

namespace yenizil::config {                               // Ürün ayarları

inline constexpr uint32_t kHeartbeatIntervalMs = 1000;    // Dış ünitenin "buradayım" yayın aralığı (ms): iç ünitelerin bağlantı LED'i her yayında bir kez yanar

}  // namespace yenizil::config
