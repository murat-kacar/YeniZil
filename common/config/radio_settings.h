#pragma once

#include <cstdint>

namespace yenizil {

struct RadioSettings {                                    // Radyo ayarları, tüm ünitelerde aynı olmalı
  uint8_t  channel;                                       // Wi-Fi kanalı (1-13)
  int8_t   txPowerDbm;                                    // Gönderim gücü (dBm), 2-20
  bool     longRange;                                     // Espressif Long Range modu
  uint16_t wakeIntervalMs;                                // Radyonun uyanma aralığı (ms)
  uint16_t wakeWindowMs;                                  // Her aralıkta dinlenen süre (ms), arası modem uykusu
};

struct BurstSettings {                                    // Tekrarlı gönderim ayarları
  uint16_t periodMs;                                      // Bir çerçevenin kopyaları arasındaki süre (ms)
  uint16_t durationMs;                                    // Bir çerçevenin tekrarlanma süresi (ms)
  uint16_t relayJitterMaxMs;                              // Aktarmadan önceki en fazla rastgele bekleme (ms)
};

}  // namespace yenizil
