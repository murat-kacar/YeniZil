#pragma once

#include "../Common/config/input_config.h"
#include "../Common/io/press_detector.h"

namespace config {                                        // İç ünite ayarları

inline constexpr uint32_t kBellPulseMs = 1500;            // Zil tetik süresi (ms)

inline constexpr PressConfig kOpenDoorPress = {           // Kapıyı aç butonu kuralları
    .minMs      = kMinPressMs,
    .maxMs      = kMaxPressMs,
    .cooldownMs = 2000,                                   // Yeni kapı açma isteği için bekleme (ms)
};

static_assert(kBellPulseMs >= 1000 && kBellPulseMs <= 2000, "Zil tetik süresi 1-2 sn olmalı (gereksinim)");

}  // namespace config
