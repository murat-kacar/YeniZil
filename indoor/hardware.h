#pragma once

#include <Arduino.h>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include "../common/io/board_pins.h"
#include "../common/kernel/static_checks.h"

namespace yenizil::pins {                                 // İç ünite kablolaması

inline constexpr uint8_t kOpenDoorButton = 10;            // Kapıyı aç butonu: pin - buton - GND
inline constexpr uint8_t kButtonActive   = LOW;           // Buton basılıyken LOW okunur (dahili pull-up)
inline constexpr uint8_t kBell           = 0;             // Zil tetiği: 3.3V, pin-GND arası 10k pull-down şart
inline constexpr uint8_t kBellActive     = HIGH;          // Zil tetik seviyesi
inline constexpr uint8_t kLinkLed        = 1;             // Bağlantı LED'i: pin - 330Ω - LED (+ bacak) - LED (- bacak) - GND
inline constexpr uint8_t kLinkLedActive  = HIGH;          // LED'i yakan seviye

inline constexpr uint8_t kUsedPins[] = {kOpenDoorButton, kBell, kLinkLed};  // Kullanılan tüm pinler, denetim için

static_assert(allUnique(kUsedPins, [](uint8_t pin) { return pin; }), "Aynı pine iki eleman bağlanamaz");
static_assert(std::all_of(std::begin(kUsedPins), std::end(kUsedPins), board::isSafeGpio), "Pin strapping/USB/UART pinine denk geliyor");

}  // namespace yenizil::pins
