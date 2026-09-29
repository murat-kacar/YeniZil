#pragma once

#include <esp_timer.h>
#include <cstdint>

namespace yenizil {

inline uint64_t monotonicMs() { return static_cast<uint64_t>(esp_timer_get_time()) / 1000; }  // Açılıştan beri geçen süre (ms), 64-bit olduğu için taşmaz. Arduino millis() 32-bit, 49,7 günde taşar

}  // namespace yenizil
