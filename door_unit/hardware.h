#pragma once

#include <Arduino.h>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include "../common/io/board_pins.h"
#include "../common/io/id_pin.h"
#include "../common/kernel/static_checks.h"
#include "../common/net/protocol.h"

namespace yenizil::pins {                                 // Kapı ünitesi (dış2) kablolaması

inline constexpr IdPin<NodeId> kFlatLeds[] = {            // Daire durum LED'leri: pin - 240Ω - LED (+ bacak) - LED (- bacak) - GND
    {.id = NodeId{1}, .pin = 1},
    {.id = NodeId{2}, .pin = 0},
    {.id = NodeId{3}, .pin = 3},
    {.id = NodeId{4}, .pin = 4},
    {.id = NodeId{5}, .pin = 5},
    {.id = NodeId{6}, .pin = 6},
    {.id = NodeId{7}, .pin = 7},
};
inline constexpr uint8_t kFlatLedActive   = HIGH;         // LED'i yakan seviye
inline constexpr uint8_t kDoorRelay       = 10;           // Kapı rölesi tetiği: 3.3V, pin-GND arası 10k pull-down şart
inline constexpr uint8_t kDoorRelayActive = HIGH;         // Röle tetik seviyesi
inline constexpr uint8_t kPairingButton   = 20;           // Eşleştirme butonu: pin - buton - GND
inline constexpr uint8_t kButtonActive    = LOW;          // Buton basılıyken LOW okunur (dahili pull-up)

static_assert(allUnique(kFlatLeds, [](const IdPin<NodeId>& led) { return led.pin; }), "Aynı pine iki LED bağlanamaz");
static_assert(allUnique(kFlatLeds, [](const IdPin<NodeId>& led) { return led.id; }), "Aynı daireye iki LED atanamaz");
static_assert(std::none_of(std::begin(kFlatLeds), std::end(kFlatLeds), [](const IdPin<NodeId>& led) { return led.pin == kDoorRelay || led.pin == kPairingButton; }), "Röle ya da eşleştirme butonu pini bir LED'le çakışıyor");
static_assert(kPairingButton != kDoorRelay, "Eşleştirme butonu röle pinine bağlanamaz");
static_assert(std::all_of(std::begin(kFlatLeds), std::end(kFlatLeds), [](const IdPin<NodeId>& led) { return board::isSafeGpio(led.pin); }), "LED pini strapping/USB/UART pinine denk geliyor");
static_assert(board::isSafeGpio(kDoorRelay), "Röle pini strapping/USB/UART pinine denk geliyor");
static_assert(board::isSafeGpio(kPairingButton), "Eşleştirme butonu pini strapping/USB/UART pinine denk geliyor");

}  // namespace yenizil::pins
