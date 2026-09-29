#pragma once

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
#include "../Common/io/button.h"
#include "../Common/app/bell.h"
#include "../Common/app/intercom.h"

inline PowerManager  powerManager(config::kCpuMhz);                                                       // Güç ayarları
inline EspNowRadio   radio(config::kRadio);                                                               // Radyo
inline CcmCipher     cipher(site::kApartmentKey);                                                         // AES-CCM, apartman anahtarıyla
inline CounterStore  counterStore;                                                                        // Sayaç kalıcı kaydı
inline SecureChannel secureChannel(cipher, counterStore);                                                 // Çerçeve üretimi ve doğrulama
inline FloodRouter   router(radio, secureChannel, NodeRole::kIndoor);                                     // Ağ
inline Intercom      intercom(router);                                                                    // Diyafon
inline Button        openDoorButton(pins::kOpenDoorButton, pins::kButtonActive, config::kOpenDoorPress);  // Kapıyı aç butonu
inline Bell          bell(pins::kBell, pins::kBellActive, config::kBellPulseMs);                          // Zil
