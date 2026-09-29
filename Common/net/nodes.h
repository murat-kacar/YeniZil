#pragma once

#include <cstddef>
#include <iterator>
#include "../config/site_config.h"
#include "../kernel/static_checks.h"
#include "protocol.h"

inline constexpr std::size_t kNodeCount = std::size(site::kNodeMacs);  // Ünite sayısı

static_assert(kNodeCount >= 2 && kNodeCount <= 16, "MAC tablosunda 2-16 ünite olmalı");
static_assert(allUnique(site::kNodeMacs, [](const MacAddress& mac) { return mac; }), "MAC tablosunda tekrar var: kimlik çakışması şifrelemeyi kırar");
static_assert(site::kApartmentId != 0, "Apartman kimliği atanmamış: site_config.h içinde kApartmentId rastgele doldurulmalı");
