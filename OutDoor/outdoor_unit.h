#pragma once

#include <algorithm>
#include <iterator>
#include "hardware.h"
#include "unit_config.h"
#include "../Common/config/power_config.h"
#include "../Common/config/radio_config.h"
#include "../Common/kernel/event_loop.h"
#include "../Common/power/power_manager.h"
#include "../Common/net/esp_now_radio.h"
#include "../Common/net/flood_router.h"
#include "../Common/security/ccm_cipher.h"
#include "../Common/security/counter_store.h"
#include "../Common/security/secure_channel.h"
#include "../Common/io/button_group.h"
#include "../Common/app/door_opener.h"
#include "../Common/app/intercom.h"

static_assert(std::all_of(std::begin(pins::kFlatButtons), std::end(pins::kFlatButtons), [](const ButtonPin& button) { return button.id != kOutdoorUnitId && button.id < kNodeCount; }),
              "Daire butonu MAC tablosunda olmayan bir daireye bağlı");

inline PowerManager  powerManager(config::kCpuMhz);                                                  // Güç ayarları
inline EspNowRadio   radio(config::kRadio);                                                          // Radyo
inline CcmCipher     cipher(site::kApartmentKey);                                                    // AES-CCM, apartman anahtarıyla
inline CounterStore  counterStore;                                                                   // Sayaç kalıcı kaydı
inline SecureChannel secureChannel(cipher, counterStore);                                            // Çerçeve üretimi ve doğrulama
inline FloodRouter   router(radio, secureChannel, NodeRole::kOutdoor);                               // Ağ
inline Intercom      intercom(router);                                                               // Diyafon
inline ButtonGroup   flatButtons(pins::kFlatButtons, pins::kButtonActive, config::kBellPress);       // Daire zil butonları
inline DoorOpener    doorOpener(pins::kDoorRelay, pins::kDoorRelayActive, config::kDoorPulseMs);     // Kapı açıcı
