#pragma once

#include <cstdint>
#include "../common/net/protocol.h"

namespace yenizil::config {                               // Yüklemeden önce ayarlanan değerler

inline constexpr NetworkId kPanelLinkId{0x4B853D65};     // Zil paneli bağlantısının kimliği: kapı ünitesiyle aynı, 0 olamaz
inline constexpr uint8_t   kPanelLinkKey[16] = {0x2F, 0x01, 0xF4, 0x27, 0xF2, 0xA5, 0x35, 0xB5, 0x53, 0x41, 0x6E, 0xE1, 0xEE, 0x37, 0x9A, 0x45};  // Zil paneli bağlantısının şifresi (AES-128, 16 bayt): kapı ünitesiyle aynı, gizli

}  // namespace yenizil::config
