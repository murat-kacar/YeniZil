#pragma once

#include <stdint.h>
#include <esp_timer.h>

inline uint64_t monotonicMs() { return static_cast<uint64_t>(esp_timer_get_time()) / 1000; }  // Açılıştan beri geçen süre (ms), 64-bit olduğu için taşmaz
