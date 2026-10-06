#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>
#include "hardware.h"
#include "door_unit_config.h"
#include "../common/app/door_opener.h"
#include "../common/app/intercom.h"
#include "../common/app/pairing.h"
#include "../common/app/panel_link.h"
#include "../common/app/radio_stack.h"
#include "../common/config/input_config.h"
#include "../common/config/link_config.h"
#include "../common/config/power_config.h"
#include "../common/config/press_settings.h"
#include "../common/config/radio_config.h"
#include "../common/io/button.h"
#include "../common/io/id_pin.h"
#include "../common/kernel/event_loop.h"
#include "../common/kernel/periodic_timer.h"
#include "../common/net/nodes.h"
#include "../common/net/protocol.h"
#include "../common/power/power_manager.h"
#include "../common/security/network_credentials.h"

namespace yenizil::config {                               // Kapı ünitesi (dış2) ayarları

inline constexpr NodeId   kNodeId          = kDoorUnitId; // Kapı ünitesinin bina ağındaki numarası: her zaman 0
inline constexpr uint32_t kDoorPulseMs     = 1500;        // Kapı rölesi tetik süresi (ms)
inline constexpr uint32_t kPairingWindowMs = 180000;      // Eşleştirme modunun açık kalma süresi (ms): Zigbee BDB bdbcMinCommissioningTime 180 sn

inline constexpr PressSettings kPairingPress = {          // Eşleştirme butonu kuralları: 10-15 sn basılı tutulup bırakılınca
    .minMs      = 10000,                                  // Kazara basışla açılmasın
    .maxMs      = 15000,                                  // Daha uzunu sıkışmış buton sayılır
    .cooldownMs = 0,                                      // Mod açıkken tekrar açmak süreyi baştan başlatır
};

inline constexpr NetworkCredentials kNetwork = {          // Bina ağı: kapı ünitesi + iç üniteler
    .id  = kNetworkId,
    .key = kNetworkKey,
};

inline constexpr NetworkCredentials kPanelLink = {        // Zil paneliyle bağlantı
    .id  = kPanelLinkId,
    .key = kPanelLinkKey,
};

static_assert(kDoorPulseMs >= 1000 && kDoorPulseMs <= 2000, "Kapı tetik süresi 1-2 sn olmalı (gereksinim)");
static_assert(kPairingPress.minMs >= kMinPressMs && kPairingPress.minMs < kPairingPress.maxMs, "Eşleştirme basışının en kısa süresi en uzundan kısa olmalı");
static_assert(kPairingWindowMs > kPairingPress.maxMs, "Eşleştirme modu basış süresinden uzun açık kalmalı");
static_assert(isAssigned(kNetwork), "Ağ kimliği ya da şifresi atanmamış: door_unit_config.h içinde rastgele doldurulmalı");
static_assert(isAssigned(kPanelLink), "Bağlantı kimliği ya da şifresi atanmamış: door_unit_config.h içinde rastgele doldurulmalı");
static_assert(isSeparate(kNetwork, kPanelLink), "Bina ağı ile zil paneli bağlantısının kimliği ve şifresi farklı olmalı: panel ele geçse kapı açılmasın");
static_assert(std::all_of(std::begin(pins::kFlatLeds), std::end(pins::kFlatLeds), [](const IdPin<NodeId>& led) { return isFlatId(led.id); }),
              "Daire LED'i olmayan bir daireye bağlı: building_config.h kFlatCount");

}  // namespace yenizil::config

namespace yenizil {                                       // Kapı ünitesi nesneleri, tanımlanma sırasıyla başlar

inline EventLoop     eventLoop;                                                                       // Olay döngüsü
inline PowerManager  powerManager(config::kCpuMhz);                                                   // Güç ayarları, radyodan önce
inline RadioStack    radioStack(config::kRadio, config::kBurst, eventLoop);                           // Radyo ve sayaçlar, iki ağda ortak
inline Intercom      intercom(radioStack, config::kNetwork, config::kNodeId);                         // Diyafon: bina ağı
inline PanelLink     panelLink(radioStack, config::kPanelLink);                                       // Zil paneliyle bağlantı
inline DoorOpener    doorOpener(pins::kDoorRelay, pins::kDoorRelayActive, config::kDoorPulseMs);      // Kapı açıcı
inline PeriodicTimer heartbeatTimer(config::kHeartbeatIntervalMs);                                    // "Buradayım" yayın zamanlayıcısı
inline Button        pairingButton(pins::kPairingButton, pins::kButtonActive, config::kPairingPress); // Eşleştirme butonu
inline Pairing       pairing(config::kPairingWindowMs);                                               // Eşleştirme modu

}  // namespace yenizil
