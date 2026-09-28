#pragma once

#include "../net/esp_now_radio.h"

namespace config {                                        // Ürün ayarları

inline constexpr RadioConfig kRadio = {                   // Radyo ayarları, tüm ünitelerde aynı olmalı
    .channel    = 1,                                      // Wi-Fi kanalı (1-13), sahada en boş kanal seçilecek
    .txPowerDbm = 8,                                      // Gönderim gücü (dBm): düşük tüketim, Super Mini kararlılığı
    .longRange  = true,                                   // Espressif Long Range, 250 kbps: daha uzun menzil
};

static_assert(kRadio.channel >= 1 && kRadio.channel <= 13, "Kanal 1-13 olmalı");
static_assert(kRadio.txPowerDbm >= 2 && kRadio.txPowerDbm <= 20, "Gönderim gücü 2-20 dBm olmalı (sürücü sınırı)");

}  // namespace config
