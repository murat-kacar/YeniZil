#pragma once

#include "hardware.h"
#include "unit_config.h"
#include "../Common/config/power_config.h"
#include "../Common/kernel/event_loop.h"
#include "../Common/power/power_manager.h"
#include "../Common/io/button_group.h"
#include "../Common/app/door_opener.h"

inline PowerManager powerManager(config::kCpuMhz);                                                  // Güç ayarları
inline ButtonGroup  flatButtons(pins::kFlatButtons, pins::kButtonActive, config::kBellPress);      // Daire zil butonları
inline DoorOpener   doorOpener(pins::kDoorRelay, pins::kDoorRelayActive, config::kDoorPulseMs);    // Kapı açıcı
