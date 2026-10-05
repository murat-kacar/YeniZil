#pragma once

#include <Arduino.h>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include "../common/io/board_pins.h"
#include "../common/io/button_pin.h"
#include "../common/kernel/static_checks.h"
#include "../common/net/protocol.h"

namespace yenizil::pins {                                 // Zil paneli (dış1) kablolaması

inline constexpr ButtonPin<NodeId> kFlatButtons[] = {     // Zil butonları: pin - buton - GND
    {.id = NodeId{1}, .pin = 3},
    {.id = NodeId{2}, .pin = 4},
    {.id = NodeId{3}, .pin = 5},
    {.id = NodeId{4}, .pin = 6},
};
inline constexpr uint8_t kButtonActive = LOW;             // Buton basılıyken LOW okunur (dahili pull-up)

static_assert(allUnique(kFlatButtons, [](const ButtonPin<NodeId>& button) { return button.pin; }), "Aynı pine iki buton bağlanamaz");
static_assert(allUnique(kFlatButtons, [](const ButtonPin<NodeId>& button) { return button.id; }), "Aynı daireye iki buton atanamaz");
static_assert(std::all_of(std::begin(kFlatButtons), std::end(kFlatButtons), [](const ButtonPin<NodeId>& button) { return board::isSafeGpio(button.pin); }), "Buton pini strapping/USB/UART pinine denk geliyor");

}  // namespace yenizil::pins
