#pragma once

#include "indoor_config.h"
#include "hardware.h"
#include "../common/config/input_config.h"
#include "../common/config/link_config.h"
#include "../common/config/power_config.h"
#include "../common/config/radio_config.h"
#include "../common/kernel/event_loop.h"
#include "../common/power/power_manager.h"
#include "../common/net/esp_now_radio.h"
#include "../common/net/flood_router.h"
#include "../common/net/nodes.h"
#include "../common/security/ccm_cipher.h"
#include "../common/security/counter_store.h"
#include "../common/security/secure_channel.h"
#include "../common/io/button.h"
#include "../common/io/indicator_led.h"
#include "../common/io/press_detector.h"
#include "../common/app/bell.h"
#include "../common/app/intercom.h"
#include "../common/app/link_monitor.h"

namespace config {                                        // İç ünite ayarları

inline constexpr uint32_t kBellPulseMs = 1500;            // Zil tetik süresi (ms)
inline constexpr uint32_t kLinkBlinkMs = 500;             // Bağlantı yokken LED'in yanık ve sönük kalma süresi (ms)

inline constexpr PressConfig kOpenDoorPress = {           // Kapıyı aç butonu kuralları
    .minMs      = kMinPressMs,
    .maxMs      = kMaxPressMs,
    .cooldownMs = 2000,                                   // Yeni kapı açma isteği için bekleme (ms)
};

static_assert(kBellPulseMs >= 1000 && kBellPulseMs <= 2000, "Zil tetik süresi 1-2 sn olmalı (gereksinim)");
static_assert(kApartmentId != 0, "Ağ kimliği atanmamış: indoor_config.h içinde kApartmentId rastgele doldurulmalı");
static_assert(isAssignedKey(kApartmentKey), "Ağ şifresi atanmamış: indoor_config.h içinde kApartmentKey rastgele doldurulmalı");
static_assert(isFlatId(kFlatId), "Daire numarası 1..kFlatCount aralığında olmalı: indoor_config.h kFlatId");

}  // namespace config

inline PowerManager  powerManager(config::kCpuMhz);                                                       // Güç ayarları
inline EspNowRadio   radio(config::kRadio);                                                               // Radyo
inline CcmCipher     cipher(config::kApartmentKey);                                                       // AES-CCM, ağ şifresiyle
inline CounterStore  counterStore;                                                                        // Sayaç kalıcı kaydı
inline SecureChannel secureChannel(cipher, counterStore, config::kApartmentId);                           // Çerçeve üretimi ve doğrulama
inline FloodRouter   router(radio, secureChannel, config::kFlatId);                                       // Ağ: bu dairenin numarasıyla
inline Intercom      intercom(router);                                                                    // Diyafon
inline Button        openDoorButton(pins::kOpenDoorButton, pins::kButtonActive, config::kOpenDoorPress);  // Kapıyı aç butonu
inline Bell          bell(pins::kBell, pins::kBellActive, config::kBellPulseMs);                          // Zil
inline IndicatorLed  linkLed(pins::kLinkLed, pins::kLinkLedActive, config::kLinkBlinkMs);                 // Bağlantı LED'i
inline LinkMonitor   linkMonitor(config::kLinkTimeoutMs);                                                 // Bağlantı denetimi, LED'den sonra başlamalı
