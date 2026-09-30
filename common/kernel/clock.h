#pragma once

#include <esp_timer.h>
#include <cstdint>

namespace yenizil {

inline constexpr uint64_t kMicrosPerMs = 1000;           // µs → ms

inline uint64_t monotonicMs() { return static_cast<uint64_t>(esp_timer_get_time()) / kMicrosPerMs; }  // Açılıştan beri geçen süre (ms), 64-bit olduğu için taşmaz. Arduino millis() 32-bit, 49,7 günde taşar

}  // namespace yenizil
