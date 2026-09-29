#pragma once

#include <cstddef>
#include <cstdint>
#include "../../Common/security/replay_window.h"

namespace replay_window_tests {                           // ReplayWindow, derleme zamanında çalışır

template <std::size_t N>
constexpr int acceptedCount(const uint32_t (&counters)[N]) {  // Sırayla gelen sayaçlardan kaç tanesi kabul edilir
  ReplayWindow window;
  int count = 0;
  for (uint32_t counter : counters) {
    if (!window.isFresh(counter)) continue;
    window.markSeen(counter);
    ++count;
  }
  return count;
}

static_assert(acceptedCount({0, 1, 2, 3}) == 4, "Artan sayaçlar kabul edilmeli");
static_assert(acceptedCount({5, 5, 5}) == 1, "Aynı sayacın kopyaları bir kez kabul edilmeli (flooding)");
static_assert(acceptedCount({10, 8, 9, 8}) == 3, "Pencere içinde sırası karışan sayaçlar bir kez kabul edilmeli");
static_assert(acceptedCount({100, 37}) == 2, "Pencere içindeki eski sayaç (fark 63) kabul edilmeli");
static_assert(acceptedCount({100, 36}) == 1, "Pencerenin gerisindeki sayaç (fark 64) reddedilmeli");
static_assert(acceptedCount({0, 1000, 999, 1000}) == 3, "Büyük sıçramadan sonra pencere yeniden kurulmalı");
static_assert(acceptedCount({0, 200, 1}) == 2, "Sıçramadan sonra çok eski sayaç reddedilmeli");

}  // namespace replay_window_tests
