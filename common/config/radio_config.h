#pragma once

#include <cstdint>
#include "radio_settings.h"

namespace yenizil::config {                               // Ürün ayarları

inline constexpr uint8_t kChannel    = 1;                 // Wi-Fi kanalı (1-13), tüm ünitelerde aynı: sahada en boş kanal seçilir
inline constexpr int8_t  kTxPowerDbm = 8;                 // Gönderim gücü (dBm): menzil yetmezse artırılır
inline constexpr bool    kLongRange  = true;              // Espressif Long Range 250 kbps: daha uzun menzil, tüm ünitelerde aynı

inline constexpr RadioSettings kRadio = {                 // Radyo ayarları, tüm ünitelerde aynı olmalı
    .channel    = kChannel,
    .txPowerDbm = kTxPowerDbm,
    .longRange  = kLongRange,
};

inline constexpr BurstSettings kBurst = {                 // Tekrarlı gönderim ayarları
    .copies           = 3,                                // Kopya sayısı: onaysız yayında tek kaybın mesajı düşürmemesi için, BLE Mesh ağ tekrarı gibi
    .intervalMs       = 20,                               // Kopyalar arası süre (ms): kısa parazit iki kopyayı birden bozmasın
    .relayJitterMaxMs = 10,                               // Aktarmadan önce en fazla rastgele bekleme (ms): aktarıcılar çakışmasın
};

static_assert(kChannel >= 1 && kChannel <= 13, "Kanal 1-13 olmalı");
static_assert(kTxPowerDbm >= 2 && kTxPowerDbm <= 20, "Gönderim gücü 2-20 dBm olmalı (sürücü sınırı)");
static_assert(kBurst.copies >= 1 && kBurst.copies <= 5, "Kopya sayısı 1-5 olmalı: fazlası kanalı doldurur");

}  // namespace yenizil::config
