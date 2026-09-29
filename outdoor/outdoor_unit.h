#pragma once

#include <algorithm>
#include <iterator>
#include "outdoor_config.h"
#include "hardware.h"
#include "../common/config/input_config.h"
#include "../common/config/link_config.h"
#include "../common/config/power_config.h"
#include "../common/config/radio_config.h"
#include "../common/kernel/event_loop.h"
#include "../common/kernel/periodic_timer.h"
#include "../common/power/power_manager.h"
#include "../common/net/esp_now_radio.h"
#include "../common/net/flood_router.h"
#include "../common/net/nodes.h"
#include "../common/security/ccm_cipher.h"
#include "../common/security/counter_store.h"
#include "../common/security/secure_channel.h"
#include "../common/io/button_group.h"
#include "../common/io/press_detector.h"
#include "../common/app/door_opener.h"
#include "../common/app/intercom.h"

namespace config {                                        // Dış ünite ayarları

inline constexpr NodeId   kNodeId      = kOutdoorUnitId;  // Dış ünitenin ağdaki numarası: her zaman 0
inline constexpr uint32_t kDoorPulseMs = 1500;            // Kapı rölesi tetik süresi (ms)

inline constexpr PressConfig kBellPress = {               // Zil butonu kuralları
    .minMs      = kMinPressMs,
    .maxMs      = kMaxPressMs,
    .cooldownMs = 3000,                                   // Aynı daireye yeni zil için bekleme (ms): 1,5 sn darbe + 1,5 sn sessizlik
};

static_assert(kDoorPulseMs >= 1000 && kDoorPulseMs <= 2000, "Kapı tetik süresi 1-2 sn olmalı (gereksinim)");
static_assert(kApartmentId != 0, "Ağ kimliği atanmamış: outdoor_config.h içinde kApartmentId rastgele doldurulmalı");
static_assert(isAssignedKey(kApartmentKey), "Ağ şifresi atanmamış: outdoor_config.h içinde kApartmentKey rastgele doldurulmalı");
static_assert(std::all_of(std::begin(pins::kFlatButtons), std::end(pins::kFlatButtons), [](const ButtonPin& button) { return isFlatId(button.id); }),
              "Daire butonu olmayan bir daireye bağlı: building_config.h kFlatCount");

}  // namespace config

inline PowerManager  powerManager(config::kCpuMhz);                                                  // Güç ayarları
inline EspNowRadio   radio(config::kRadio);                                                          // Radyo
inline CcmCipher     cipher(config::kApartmentKey);                                                  // AES-CCM, ağ şifresiyle
inline CounterStore  counterStore;                                                                   // Sayaç kalıcı kaydı
inline SecureChannel secureChannel(cipher, counterStore, config::kApartmentId);                      // Çerçeve üretimi ve doğrulama
inline FloodRouter   router(radio, secureChannel, config::kNodeId);                                  // Ağ
inline Intercom      intercom(router);                                                               // Diyafon
inline ButtonGroup   flatButtons(pins::kFlatButtons, pins::kButtonActive, config::kBellPress);       // Daire zil butonları
inline DoorOpener    doorOpener(pins::kDoorRelay, pins::kDoorRelayActive, config::kDoorPulseMs);     // Kapı açıcı
inline PeriodicTimer heartbeatTimer(config::kHeartbeatIntervalMs);                                   // "Buradayım" yayın zamanlayıcısı
