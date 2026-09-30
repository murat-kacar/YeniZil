#pragma once

#include <cstddef>
#include "../config/building_config.h"
#include "../kernel/enum_value.h"
#include "protocol.h"

namespace yenizil {

inline constexpr std::size_t kNodeCount = config::kFlatCount + 1u;  // Ünite sayısı: dış ünite + daireler

constexpr bool isNode(NodeId id) { return toUnderlying(id) < kNodeCount; }             // Binadaki bir ünite mi
constexpr bool isFlatId(NodeId id) { return id != kOutdoorUnitId && isNode(id); }       // Geçerli bir daire numarası mı
constexpr bool isDestination(NodeId id) { return isNode(id) || id == kAllUnitsId; }     // Geçerli bir hedef mi

}  // namespace yenizil
