#pragma once

#include <cstddef>
#include "../config/building_config.h"
#include "protocol.h"

namespace yenizil {

inline constexpr std::size_t kNodeCount = config::kFlatCount + 1u;  // Ünite sayısı: dış ünite + daireler

constexpr bool isFlatId(NodeId id) { return id != kOutdoorUnitId && id < kNodeCount; }  // Geçerli bir daire numarası mı
constexpr bool isDestination(NodeId id) { return id < kNodeCount || id == kAllUnitsId; }  // Geçerli bir hedef mi

}  // namespace yenizil
