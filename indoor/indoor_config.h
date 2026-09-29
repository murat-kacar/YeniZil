#pragma once

#include <cstdint>

namespace config {                                        // Yüklemeden önce ayarlanan değerler

inline constexpr uint32_t kApartmentId     = 0x591973EE;  // Ağ (apartman) kimliği: tüm ünitelerde aynı, 0 olamaz
inline constexpr uint8_t  kApartmentKey[16] = {0xF6, 0x46, 0x57, 0x04, 0x47, 0x3C, 0x27, 0x9B, 0xCF, 0xB6, 0x06, 0x9E, 0x51, 0x7D, 0xC2, 0xB0};  // Ağ şifresi (AES-128): tüm ünitelerde aynı, gizli
inline constexpr uint8_t  kFlatId          = 1;           // Bu iç ünitenin daire numarası (1..kFlatCount)

}  // namespace config
