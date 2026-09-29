#pragma once

#include <cstddef>
#include "../config/site_config.h"
#include "protocol.h"

inline constexpr std::size_t kNodeCount = site::kFlatCount + 1u;  // Ünite sayısı: dış ünite + daireler

static_assert(site::kFlatCount >= 1 && site::kFlatCount <= 15, "Daire sayısı 1-15 olmalı");
static_assert(site::kApartmentId != 0, "Apartman kimliği atanmamış: site_config.h içinde kApartmentId rastgele doldurulmalı");

constexpr bool isFlatId(NodeId id) { return id != kOutdoorUnitId && id < kNodeCount; }  // Geçerli bir daire kimliği mi
