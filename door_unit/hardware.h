#pragma once

#include <Arduino.h>
#include <cstdint>
#include "../common/io/board_pins.h"

namespace yenizil::pins {                                 // Kapı ünitesi (dış2) kablolaması

inline constexpr uint8_t kDoorRelay       = 10;           // Kapı rölesi tetiği: 3.3V, pin-GND arası 10k pull-down şart
inline constexpr uint8_t kDoorRelayActive = HIGH;         // Röle tetik seviyesi

static_assert(board::isSafeGpio(kDoorRelay), "Röle pini strapping/USB/UART pinine denk geliyor");

}  // namespace yenizil::pins
