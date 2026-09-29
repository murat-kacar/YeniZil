#pragma once

#include "../net/esp_now_radio.h"
#include "site_config.h"

namespace config {                                        // Ürün ayarları

inline constexpr uint16_t kWakeIntervalMs = 200;          // Uyanma aralığı (ms), Aşama 4: Espressif 100'ün katlarını öneriyor
inline constexpr uint16_t kWakeWindowMs   = 20;           // Uyanık kalma penceresi (ms), Aşama 4: %10 görev oranı

inline constexpr RadioConfig kRadio = {                   // Radyo ayarları, tüm ünitelerde aynı olmalı
    .channel          = site::kChannel,
    .txPowerDbm       = site::kTxPowerDbm,
    .longRange        = site::kLongRange,
    .burstPeriodMs    = 10,                                   // Tekrar aralığı (ms): pencere başına en az 2 kopya
    .burstDurationMs  = 2 * kWakeIntervalMs + kWakeWindowMs,  // Tekrar süresi (ms): alıcı 2 pencere görür, kat başına kaçırma ≈ %0,01
    .relayJitterMaxMs = 10,                                   // Aktarmadan önce en fazla rastgele bekleme (ms): aktarıcılar çakışmasın
};

static_assert(kRadio.channel >= 1 && kRadio.channel <= 13, "Kanal 1-13 olmalı");
static_assert(kRadio.txPowerDbm >= 2 && kRadio.txPowerDbm <= 20, "Gönderim gücü 2-20 dBm olmalı (sürücü sınırı)");
static_assert(kWakeWindowMs < kWakeIntervalMs, "Pencere aralıktan kısa olmalı");
static_assert(kRadio.burstPeriodMs * 2 <= kWakeWindowMs, "Pencereye en az 2 kopya düşmeli");
static_assert(kRadio.burstDurationMs >= 2 * kWakeIntervalMs, "Alıcı en az 2 pencere görmeli");

}  // namespace config
