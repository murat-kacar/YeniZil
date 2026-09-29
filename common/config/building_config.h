#pragma once

#include <cstdint>

namespace yenizil::config {                               // Bina ayarları

inline constexpr uint8_t kFlatCount = 4;                  // Daire sayısı: iç ünite numaraları 1..kFlatCount, dış ünite 0

static_assert(kFlatCount >= 1 && kFlatCount <= 15, "Daire sayısı 1-15 olmalı");

}  // namespace yenizil::config
