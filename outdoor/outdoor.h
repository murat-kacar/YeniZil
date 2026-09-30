#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>
#include "hardware.h"
#include "outdoor_config.h"
#include "../common/app/door_opener.h"
#include "../common/app/intercom.h"
#include "../common/config/input_config.h"
#include "../common/config/link_config.h"
#include "../common/config/power_config.h"
#include "../common/config/press_settings.h"
#include "../common/config/radio_config.h"
#include "../common/io/button_group.h"
#include "../common/io/button_pin.h"
#include "../common/kernel/event_loop.h"
#include "../common/kernel/periodic_timer.h"
#include "../common/net/nodes.h"
#include "../common/net/protocol.h"
#include "../common/power/power_manager.h"

namespace yenizil::config {                               // Dış ünite ayarları

inline constexpr NodeId   kNodeId      = kOutdoorUnitId;  // Dış ünitenin ağdaki numarası: her zaman 0
inline constexpr uint32_t kDoorPulseMs = 1500;            // Kapı rölesi tetik süresi (ms)

inline constexpr PressSettings kBellPress = {             // Zil butonu kuralları
    .minMs      = kMinPressMs,
    .maxMs      = kMaxPressMs,
    .cooldownMs = 3000,                                   // Aynı daireye yeni zil için bekleme (ms): 1,5 sn darbe + 1,5 sn sessizlik
};

inline constexpr NetworkSettings kNetwork = {             // Ağ ayarları
    .radio       = kRadio,
    .burst       = kBurst,
    .apartmentId = kApartmentId,
    .key         = kApartmentKey,
    .nodeId      = kNodeId,
};

static_assert(kDoorPulseMs >= 1000 && kDoorPulseMs <= 2000, "Kapı tetik süresi 1-2 sn olmalı (gereksinim)");
static_assert(isAssigned(kNetwork), "Ağ kimliği ya da şifresi atanmamış: outdoor_config.h içinde rastgele doldurulmalı");
static_assert(std::all_of(std::begin(pins::kFlatButtons), std::end(pins::kFlatButtons), [](const ButtonPin<NodeId>& button) { return isFlatId(button.id); }),
              "Daire butonu olmayan bir daireye bağlı: building_config.h kFlatCount");

}  // namespace yenizil::config

namespace yenizil {                                       // Dış ünite nesneleri, tanımlanma sırasıyla başlar

inline EventLoop     eventLoop;                                                                      // Olay döngüsü
inline PowerManager  powerManager(config::kCpuMhz);                                                  // Güç ayarları, radyodan önce
inline Intercom      intercom(config::kNetwork, eventLoop);                                          // Diyafon ve ağ
inline ButtonGroup   flatButtons(pins::kFlatButtons, pins::kButtonActive, config::kBellPress);       // Daire zil butonları
inline DoorOpener    doorOpener(pins::kDoorRelay, pins::kDoorRelayActive, config::kDoorPulseMs);     // Kapı açıcı
inline PeriodicTimer heartbeatTimer(config::kHeartbeatIntervalMs);                                   // "Buradayım" yayın zamanlayıcısı

}  // namespace yenizil
