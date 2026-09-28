#pragma once

#include <Arduino.h>
#include "../Common/io/board_pins.h"

namespace pins {                                          // İç ünite kablolaması

inline constexpr uint8_t kOpenDoorButton = 3;             // Kapıyı aç butonu: pin - buton - GND
inline constexpr uint8_t kButtonActive   = LOW;           // Buton basılıyken LOW okunur (dahili pull-up)
inline constexpr uint8_t kBell           = 10;            // Zil tetiği: 3.3V, pin-GND arası 10k pull-down şart
inline constexpr uint8_t kBellActive     = HIGH;          // Zil tetik seviyesi

static_assert(kOpenDoorButton != kBell, "Buton ve zil aynı pinde olamaz");
static_assert(board::isSafeGpio(kOpenDoorButton) && board::isSafeGpio(kBell), "Pin strapping/USB/UART pinine denk geliyor");

}  // namespace pins
