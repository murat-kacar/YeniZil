#pragma once

#include "../Common/config/input_config.h"
#include "../Common/io/press_detector.h"

namespace config {                                        // Dış ünite ayarları

inline constexpr uint32_t kDoorPulseMs = 1500;            // Kapı rölesi tetik süresi (ms)

inline constexpr PressConfig kBellPress = {               // Zil butonu kuralları
    .minMs      = kMinPressMs,
    .maxMs      = kMaxPressMs,
    .cooldownMs = 3000,                                   // Aynı daireye yeni zil için bekleme (ms): 1,5 sn darbe + 1,5 sn sessizlik
};

static_assert(kDoorPulseMs >= 1000 && kDoorPulseMs <= 2000, "Kapı tetik süresi 1-2 sn olmalı (gereksinim)");

}  // namespace config
