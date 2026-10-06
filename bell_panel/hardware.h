#pragma once

#include <Arduino.h>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include "../common/io/board_pins.h"
#include "../common/io/id_pin.h"
#include "../common/kernel/static_checks.h"
#include "../common/net/protocol.h"

namespace yenizil::pins {                                 // Zil paneli (dış1) kablolaması

inline constexpr IdPin<NodeId> kFlatButtons[] = {         // Zil butonları: pin - buton - GND
    {.id = NodeId{1}, .pin = 1},
    {.id = NodeId{2}, .pin = 0},
    {.id = NodeId{3}, .pin = 3},
    {.id = NodeId{4}, .pin = 4},
    {.id = NodeId{5}, .pin = 5},
    {.id = NodeId{6}, .pin = 6},
    {.id = NodeId{7}, .pin = 7},
};
inline constexpr uint8_t kButtonActive     = LOW;         // Buton basılıyken LOW okunur (dahili pull-up)
inline constexpr uint8_t kPanelLight       = 10;          // Panel aydınlatma LED'i: pin - 240Ω - LED (+ bacak) - LED (- bacak) - GND
inline constexpr uint8_t kPanelLightActive = HIGH;        // LED'i yakan seviye

static_assert(allUnique(kFlatButtons, [](const IdPin<NodeId>& button) { return button.pin; }), "Aynı pine iki buton bağlanamaz");
static_assert(allUnique(kFlatButtons, [](const IdPin<NodeId>& button) { return button.id; }), "Aynı daireye iki buton atanamaz");
static_assert(std::none_of(std::begin(kFlatButtons), std::end(kFlatButtons), [](const IdPin<NodeId>& button) { return button.pin == kPanelLight; }), "Aydınlatma pini bir butonla çakışıyor");
static_assert(std::all_of(std::begin(kFlatButtons), std::end(kFlatButtons), [](const IdPin<NodeId>& button) { return board::isSafeGpio(button.pin); }), "Buton pini strapping/USB/UART pinine denk geliyor");
static_assert(board::isSafeGpio(kPanelLight), "Aydınlatma pini strapping/USB/UART pinine denk geliyor");

}  // namespace yenizil::pins
