#pragma once

#include "../../Common/io/board_pins.h"
#include "../../Common/kernel/static_checks.h"

namespace static_checks_tests {                           // allUnique ve board::isSafeGpio testleri, derleme zamanında çalışır

inline constexpr int kUnique[]    = {1, 2, 3};            // Tekrarsız tablo
inline constexpr int kDuplicate[] = {1, 2, 1};            // Tekrarlı tablo

static_assert(allUnique(kUnique, [](int value) { return value; }), "Tekrarsız tablo geçmeli");
static_assert(!allUnique(kDuplicate, [](int value) { return value; }), "Tekrarlı tablo yakalanmalı");

static_assert(board::isSafeGpio(3) && board::isSafeGpio(10) && board::isSafeGpio(0), "Güvenli pinler kabul edilmeli");
static_assert(!board::isSafeGpio(2) && !board::isSafeGpio(8) && !board::isSafeGpio(9), "Strapping pinleri reddedilmeli");
static_assert(!board::isSafeGpio(18) && !board::isSafeGpio(19), "USB pinleri reddedilmeli");
static_assert(!board::isSafeGpio(20) && !board::isSafeGpio(21), "UART0 pinleri reddedilmeli");

}  // namespace static_checks_tests
