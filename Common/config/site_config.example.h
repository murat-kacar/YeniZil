#pragma once

#include <cstdint>

namespace site {                                          // Bu apartmana özgü değerler. Şablon: site_config.h adıyla kopyalanır, o dosya git dışı

inline constexpr uint32_t kApartmentId = 0x00000000;      // Apartman kimliği: rastgele, 0 olamaz
inline constexpr uint8_t kApartmentKey[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // Apartman anahtarı (AES-128): rastgele 16 bayt, tüm ünitelerde aynı, gizli

inline constexpr uint8_t kFlatCount = 4;              // Daire sayısı: iç ünite kimlikleri 1..kFlatCount, dış ünite 0

inline constexpr uint8_t kChannel    = 1;                 // Wi-Fi kanalı (1-13), sahada en boş kanal seçilir
inline constexpr int8_t  kTxPowerDbm = 8;                 // Gönderim gücü (dBm): menzil yetmezse artırılır
inline constexpr bool    kLongRange  = true;              // Espressif Long Range 250 kbps: daha uzun menzil

}  // namespace site
