#pragma once

#include <cstdint>

namespace yenizil {

struct RadioSettings {                                    // Radyo ayarları, tüm ünitelerde aynı olmalı
  uint8_t channel;                                        // Wi-Fi kanalı (1-13)
  int8_t  txPowerDbm;                                     // Gönderim gücü (dBm), 2-20
  bool    longRange;                                      // Espressif Long Range modu
};

struct BurstSettings {                                    // Tekrarlı gönderim ayarları
  uint8_t  copies;                                        // Bir çerçevenin kaç kez gönderileceği
  uint16_t intervalMs;                                    // Kopyalar arasındaki süre (ms)
  uint16_t relayJitterMaxMs;                              // Aktarmadan önceki en fazla rastgele bekleme (ms)
};

}  // namespace yenizil
