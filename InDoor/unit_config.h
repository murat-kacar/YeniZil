#pragma once

#include "../Common/config/input_config.h"
#include "../Common/config/link_config.h"
#include "../Common/io/press_detector.h"

namespace config {                                        // İç ünite ayarları

inline constexpr uint32_t kBellPulseMs     = 1500;        // Zil tetik süresi (ms)
inline constexpr uint32_t kPairingHoldMs   = 5000;        // Açılışta "kapıyı aç" en az bu kadar basılı tutulursa eşleştirme modu (ms)
inline constexpr uint32_t kPairingWindowMs = 120000;      // Eşleştirme modunun süresi (ms): bu sürede dairenin dış ünitedeki butonuna basılır
inline constexpr uint32_t kLinkBlinkMs     = 500;         // Bağlantı yokken LED'in yanık ve sönük kalma süresi (ms)

inline constexpr PressConfig kOpenDoorPress = {           // Kapıyı aç butonu kuralları
    .minMs         = kMinPressMs,
    .maxMs         = kMaxPressMs,
    .cooldownMs    = 2000,                                // Yeni kapı açma isteği için bekleme (ms)
    .startupHoldMs = kPairingHoldMs,                      // Eşleştirme modu için açılışta basılı tutma
};

static_assert(kBellPulseMs >= 1000 && kBellPulseMs <= 2000, "Zil tetik süresi 1-2 sn olmalı (gereksinim)");
static_assert(kPairingHoldMs >= 3000, "Eşleştirme için basılı tutma kazara olmayacak kadar uzun olmalı");
static_assert(kPairingWindowMs >= 30000, "Eşleştirme penceresi dış üniteye yürümeye yetecek kadar uzun olmalı");

}  // namespace config
