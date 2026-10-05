#pragma once

#include "../config/building_config.h"
#include "../kernel/enum_value.h"
#include "protocol.h"

namespace yenizil {

constexpr bool isFlatId(NodeId id) { return id != kDoorUnitId && toUnderlying(id) <= config::kFlatCount; }  // Geçerli bir daire numarası mı
constexpr bool isNode(NodeId id) { return id == kDoorUnitId || id == kBellPanelId || isFlatId(id); }        // Binadaki bir ünite mi
constexpr bool isDestination(NodeId id) { return isNode(id) || id == kAllUnitsId; }                         // Geçerli bir hedef mi

static_assert(!isFlatId(kBellPanelId) && kBellPanelId != kAllUnitsId, "Zil paneli kimliği bir daireyle ya da herkese adresiyle çakışıyor");

}  // namespace yenizil
