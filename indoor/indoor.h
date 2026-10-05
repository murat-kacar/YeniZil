#pragma once

#include <cstdint>
#include "hardware.h"
#include "indoor_config.h"
#include "../common/app/bell.h"
#include "../common/app/intercom.h"
#include "../common/app/radio_stack.h"
#include "../common/config/input_config.h"
#include "../common/config/link_config.h"
#include "../common/config/power_config.h"
#include "../common/config/press_settings.h"
#include "../common/config/radio_config.h"
#include "../common/io/button.h"
#include "../common/io/pulse_output.h"
#include "../common/kernel/event_loop.h"
#include "../common/net/nodes.h"
#include "../common/power/power_manager.h"
#include "../common/security/network_credentials.h"

namespace yenizil::config {                               // İç ünite ayarları

inline constexpr uint32_t kBellPulseMs = 1500;            // Zil tetik süresi (ms)
inline constexpr uint32_t kLinkPulseMs = 10;              // Her heartbeat'te bağlantı LED'inin yanık kalma süresi (ms)

inline constexpr PressSettings kOpenDoorPress = {         // Kapıyı aç butonu kuralları
    .minMs      = kMinPressMs,
    .maxMs      = kMaxPressMs,
    .cooldownMs = 2000,                                   // Yeni kapı açma isteği için bekleme (ms)
};

inline constexpr NetworkCredentials kNetwork = {          // Bina ağı: kapı ünitesi + iç üniteler
    .id  = kNetworkId,
    .key = kNetworkKey,
};

static_assert(kBellPulseMs >= 1000 && kBellPulseMs <= 2000, "Zil tetik süresi 1-2 sn olmalı (gereksinim)");
static_assert(kLinkPulseMs < kHeartbeatIntervalMs, "LED darbesi heartbeat aralığından kısa olmalı: yoksa sürekli yanar, kopukluk görünmez");
static_assert(isAssigned(kNetwork), "Ağ kimliği ya da şifresi atanmamış: indoor_config.h içinde rastgele doldurulmalı");
static_assert(isFlatId(kFlatId), "Daire numarası 1..kFlatCount aralığında olmalı: indoor_config.h kFlatId");

}  // namespace yenizil::config

namespace yenizil {                                       // İç ünite nesneleri, tanımlanma sırasıyla başlar

inline EventLoop     eventLoop;                                                                              // Olay döngüsü
inline PowerManager  powerManager(config::kCpuMhz);                                                          // Güç ayarları, radyodan önce
inline RadioStack    radioStack(config::kRadio, config::kBurst, eventLoop);                                  // Radyo ve sayaçlar
inline Intercom      intercom(radioStack, config::kNetwork, config::kFlatId);                                // Diyafon: bina ağı
inline Button        openDoorButton(pins::kOpenDoorButton, pins::kButtonActive, config::kOpenDoorPress);     // Kapıyı aç butonu
inline Bell          bell(pins::kBell, pins::kBellActive, config::kBellPulseMs);                             // Zil
inline PulseOutput   linkLed(pins::kLinkLed, pins::kLinkLedActive, config::kLinkPulseMs);                    // Bağlantı LED'i: her heartbeat'te kısa yanar

}  // namespace yenizil
