#pragma once

#include "hardware.h"
#include "unit_config.h"
#include "../Common/config/power_config.h"
#include "../Common/config/radio_config.h"
#include "../Common/kernel/debug_serial.h"
#include "../Common/kernel/event_loop.h"
#include "../Common/power/power_manager.h"
#include "../Common/net/esp_now_radio.h"
#include "../Common/io/button.h"
#include "../Common/app/bell.h"

inline DebugSerial  debugSerial;                                                                         // Loglar, diğer bileşenlerden önce başlamalı
inline PowerManager powerManager(config::kCpuMhz);                                                     // Güç ayarları
inline EspNowRadio  radio(config::kRadio);                                                               // Radyo
inline Button       openDoorButton(pins::kOpenDoorButton, pins::kButtonActive, config::kOpenDoorPress);  // Kapıyı aç butonu
inline Bell         bell(pins::kBell, pins::kBellActive, config::kBellPulseMs);                          // Zil
