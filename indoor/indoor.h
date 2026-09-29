#pragma once

#include <cstdint>
#include "hardware.h"
#include "indoor_config.h"
#include "../common/app/bell.h"
#include "../common/app/intercom.h"
#include "../common/app/link_monitor.h"
#include "../common/config/input_config.h"
#include "../common/config/link_config.h"
#include "../common/config/power_config.h"
#include "../common/config/press_settings.h"
#include "../common/config/radio_config.h"
#include "../common/io/button.h"
#include "../common/io/indicator_led.h"
#include "../common/kernel/event_loop.h"
#include "../common/net/nodes.h"
#include "../common/power/power_manager.h"

namespace yenizil::config {                               // İç ünite ayarları

inline constexpr uint32_t kBellPulseMs = 1500;            // Zil tetik süresi (ms)
inline constexpr uint32_t kLinkBlinkMs = 500;             // Bağlantı yokken LED'in yanık ve sönük kalma süresi (ms)

inline constexpr PressSettings kOpenDoorPress = {         // Kapıyı aç butonu kuralları
    .minMs      = kMinPressMs,
    .maxMs      = kMaxPressMs,
    .cooldownMs = 2000,                                   // Yeni kapı açma isteği için bekleme (ms)
};

inline constexpr NetworkSettings kNetwork = {             // Ağ ayarları
    .radio       = kRadio,
    .burst       = kBurst,
    .apartmentId = kApartmentId,
    .key         = kApartmentKey,
    .nodeId      = kFlatId,
};

static_assert(kBellPulseMs >= 1000 && kBellPulseMs <= 2000, "Zil tetik süresi 1-2 sn olmalı (gereksinim)");
static_assert(isAssigned(kNetwork), "Ağ kimliği ya da şifresi atanmamış: indoor_config.h içinde rastgele doldurulmalı");
static_assert(isFlatId(kFlatId), "Daire numarası 1..kFlatCount aralığında olmalı: indoor_config.h kFlatId");

}  // namespace yenizil::config

namespace yenizil {                                       // İç ünite nesneleri, tanımlanma sırasıyla başlar

inline EventLoop    eventLoop;                                                                           // Olay döngüsü
inline PowerManager powerManager(config::kCpuMhz);                                                       // Güç ayarları, radyodan önce
inline Intercom     intercom(config::kNetwork, eventLoop);                                               // Diyafon ve ağ
inline Button       openDoorButton(pins::kOpenDoorButton, pins::kButtonActive, config::kOpenDoorPress);  // Kapıyı aç butonu
inline Bell         bell(pins::kBell, pins::kBellActive, config::kBellPulseMs);                          // Zil
inline IndicatorLed linkLed(pins::kLinkLed, pins::kLinkLedActive, config::kLinkBlinkMs);                 // Bağlantı LED'i
inline LinkMonitor  linkMonitor(config::kLinkTimeoutMs);                                                 // Bağlantı denetimi

}  // namespace yenizil
