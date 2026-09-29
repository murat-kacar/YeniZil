#pragma once

#include <cstdint>
#include "radio_settings.h"

namespace yenizil::config {                               // Ürün ayarları

inline constexpr uint8_t  kChannel        = 1;            // Wi-Fi kanalı (1-13), tüm ünitelerde aynı: sahada en boş kanal seçilir
inline constexpr int8_t   kTxPowerDbm     = 8;            // Gönderim gücü (dBm): menzil yetmezse artırılır
inline constexpr bool     kLongRange      = true;         // Espressif Long Range 250 kbps: daha uzun menzil, tüm ünitelerde aynı
inline constexpr uint16_t kWakeIntervalMs = 200;          // Uyanma aralığı (ms): Espressif 100'ün katlarını öneriyor
inline constexpr uint16_t kWakeWindowMs   = 20;           // Uyanık kalma penceresi (ms): %10 görev oranı

inline constexpr RadioSettings kRadio = {                 // Radyo ayarları, tüm ünitelerde aynı olmalı
    .channel        = kChannel,
    .txPowerDbm     = kTxPowerDbm,
    .longRange      = kLongRange,
    .wakeIntervalMs = kWakeIntervalMs,
    .wakeWindowMs   = kWakeWindowMs,
};

inline constexpr BurstSettings kBurst = {                 // Tekrarlı gönderim ayarları
    .periodMs         = 10,                               // Tekrar aralığı (ms): pencere başına en az 2 kopya
    .durationMs       = 2 * kWakeIntervalMs + kWakeWindowMs,  // Tekrar süresi (ms): alıcı 2 pencere görür, kat başına kaçırma ≈ %0,01
    .relayJitterMaxMs = 10,                               // Aktarmadan önce en fazla rastgele bekleme (ms): aktarıcılar çakışmasın
};

static_assert(kChannel >= 1 && kChannel <= 13, "Kanal 1-13 olmalı");
static_assert(kTxPowerDbm >= 2 && kTxPowerDbm <= 20, "Gönderim gücü 2-20 dBm olmalı (sürücü sınırı)");
static_assert(kWakeWindowMs < kWakeIntervalMs, "Pencere aralıktan kısa olmalı");
static_assert(kBurst.periodMs * 2 <= kWakeWindowMs, "Pencereye en az 2 kopya düşmeli");
static_assert(kBurst.durationMs >= 2 * kWakeIntervalMs, "Alıcı en az 2 pencere görmeli");

}  // namespace yenizil::config
