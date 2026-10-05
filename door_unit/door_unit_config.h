#pragma once

#include <cstdint>
#include "../common/net/protocol.h"

namespace yenizil::config {                               // Yüklemeden önce ayarlanan değerler

inline constexpr NetworkId kNetworkId{0x591973EE};       // Bina ağının kimliği: iç ünitelerle aynı, 0 olamaz
inline constexpr uint8_t   kNetworkKey[16] = {0xF6, 0x46, 0x57, 0x04, 0x47, 0x3C, 0x27, 0x9B, 0xCF, 0xB6, 0x06, 0x9E, 0x51, 0x7D, 0xC2, 0xB0};  // Bina ağının şifresi (AES-128, 16 bayt): iç ünitelerle aynı, gizli
inline constexpr NetworkId kPanelLinkId{0x4B853D65};     // Zil paneli bağlantısının kimliği: zil paneliyle aynı, 0 olamaz
inline constexpr uint8_t   kPanelLinkKey[16] = {0x2F, 0x01, 0xF4, 0x27, 0xF2, 0xA5, 0x35, 0xB5, 0x53, 0x41, 0x6E, 0xE1, 0xEE, 0x37, 0x9A, 0x45};  // Zil paneli bağlantısının şifresi (AES-128, 16 bayt): zil paneliyle aynı, gizli

}  // namespace yenizil::config
