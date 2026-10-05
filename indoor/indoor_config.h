#pragma once

#include <cstdint>
#include "../common/net/protocol.h"

namespace yenizil::config {                               // Yüklemeden önce ayarlanan değerler

inline constexpr NetworkId kNetworkId{0x591973EE};       // Bina ağının kimliği: kapı ünitesi ve iç ünitelerde aynı, 0 olamaz
inline constexpr uint8_t   kNetworkKey[16] = {0xF6, 0x46, 0x57, 0x04, 0x47, 0x3C, 0x27, 0x9B, 0xCF, 0xB6, 0x06, 0x9E, 0x51, 0x7D, 0xC2, 0xB0};  // Bina ağının şifresi (AES-128, 16 bayt): kapı ünitesi ve iç ünitelerde aynı, gizli
inline constexpr NodeId    kFlatId{4};                  // Bu iç ünitenin daire numarası (1..kFlatCount)

}  // namespace yenizil::config
