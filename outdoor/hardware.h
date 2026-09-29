#pragma once

#include <Arduino.h>
#include <algorithm>
#include <iterator>
#include "../common/io/board_pins.h"
#include "../common/io/button_group.h"
#include "../common/kernel/static_checks.h"

namespace pins {                                          // Dış ünite kablolaması

inline constexpr ButtonPin kFlatButtons[] = {             // Zil butonları {daire, pin}: pin - buton - GND
    {1, 3},
    {2, 4},
    {3, 5},
    {4, 6},
};
inline constexpr uint8_t kButtonActive    = LOW;          // Buton basılıyken LOW okunur (dahili pull-up)
inline constexpr uint8_t kDoorRelay       = 10;           // Kapı rölesi tetiği: 3.3V, pin-GND arası 10k pull-down şart
inline constexpr uint8_t kDoorRelayActive = HIGH;         // Röle tetik seviyesi

static_assert(allUnique(kFlatButtons, [](const ButtonPin& button) { return button.pin; }), "Aynı pine iki buton bağlanamaz");
static_assert(allUnique(kFlatButtons, [](const ButtonPin& button) { return button.id; }), "Aynı daireye iki buton atanamaz");
static_assert(std::none_of(std::begin(kFlatButtons), std::end(kFlatButtons), [](const ButtonPin& button) { return button.pin == kDoorRelay; }), "Röle pini bir butonla çakışıyor");
static_assert(std::all_of(std::begin(kFlatButtons), std::end(kFlatButtons), [](const ButtonPin& button) { return board::isSafeGpio(button.pin); }), "Buton pini strapping/USB/UART pinine denk geliyor");
static_assert(board::isSafeGpio(kDoorRelay), "Röle pini strapping/USB/UART pinine denk geliyor");

}  // namespace pins
