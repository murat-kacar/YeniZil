#pragma once

#include <algorithm>
#include <iterator>
#include "hardware.h"
#include "bell_panel_config.h"
#include "../common/app/panel_link.h"
#include "../common/app/radio_stack.h"
#include "../common/config/input_config.h"
#include "../common/config/power_config.h"
#include "../common/config/press_settings.h"
#include "../common/config/radio_config.h"
#include "../common/io/button_group.h"
#include "../common/io/id_pin.h"
#include "../common/kernel/event_loop.h"
#include "../common/net/nodes.h"
#include "../common/net/protocol.h"
#include "../common/power/power_manager.h"
#include "../common/security/network_credentials.h"

namespace yenizil::config {                               // Zil paneli (dış1) ayarları

inline constexpr PressSettings kBellPress = {             // Zil butonu kuralları
    .minMs      = kMinPressMs,
    .maxMs      = kMaxPressMs,
    .cooldownMs = 3000,                                   // Aynı daireye yeni zil için bekleme (ms): 1,5 sn darbe + 1,5 sn sessizlik
};

inline constexpr NetworkCredentials kPanelLink = {        // Kapı ünitesiyle bağlantı: bina ağının şifresi bu kartta yok
    .id  = kPanelLinkId,
    .key = kPanelLinkKey,
};

static_assert(isAssigned(kPanelLink), "Bağlantı kimliği ya da şifresi atanmamış: bell_panel_config.h içinde rastgele doldurulmalı");
static_assert(std::all_of(std::begin(pins::kFlatButtons), std::end(pins::kFlatButtons), [](const IdPin<NodeId>& button) { return isFlatId(button.id); }),
              "Daire butonu olmayan bir daireye bağlı: building_config.h kFlatCount");

}  // namespace yenizil::config

namespace yenizil {                                       // Zil paneli nesneleri, tanımlanma sırasıyla başlar

inline EventLoop    eventLoop;                                                                  // Olay döngüsü
inline PowerManager powerManager(config::kCpuMhz);                                              // Güç ayarları, radyodan önce
inline RadioStack   radioStack(config::kRadio, config::kBurst, eventLoop);                      // Radyo ve sayaçlar
inline PanelLink    panelLink(radioStack, config::kPanelLink);                                  // Kapı ünitesiyle bağlantı
inline ButtonGroup  flatButtons(pins::kFlatButtons, pins::kButtonActive, config::kBellPress);   // Daire zil butonları

}  // namespace yenizil
