#pragma once

#include "hardware.h"
#include "unit_config.h"
#include "../Common/config/power_config.h"
#include "../Common/kernel/event_loop.h"
#include "../Common/power/power_manager.h"
#include "../Common/io/button.h"
#include "../Common/app/bell.h"

inline PowerManager powerManager(config::kCpuMhz);                                                       // Güç ayarları
inline Button       openDoorButton(pins::kOpenDoorButton, pins::kButtonActive, config::kOpenDoorPress);  // Kapıyı aç butonu
inline Bell         bell(pins::kBell, pins::kBellActive, config::kBellPulseMs);                          // Zil
