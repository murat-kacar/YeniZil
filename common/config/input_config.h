#pragma once

#include <stdint.h>

namespace config {                                        // Ürün ayarları

inline constexpr uint32_t kSamplePeriodMs = 5;            // Buton örnekleme periyodu (ms): Ganssle 1-5 ms
inline constexpr uint32_t kMinPressMs     = 50;           // En kısa geçerli basış (ms): sıçrama < 10 ms (Ganssle), EFT 15 ms (IEC 61000-4-4), insan ≈ 80-110 ms
inline constexpr uint32_t kMaxPressMs     = 30000;        // En uzun geçerli basış (ms): HMI basılı tut zaman aşımı ≥ 30 sn, tahmin

static_assert(kSamplePeriodMs >= 1 && kSamplePeriodMs <= 5, "Örnekleme periyodu 1-5 ms olmalı (Ganssle)");
static_assert(kMinPressMs >= 4 * kSamplePeriodMs, "En kısa basış en az 4 örnek sürmeli");
static_assert(kMinPressMs < kMaxPressMs, "En kısa basış en uzun basıştan kısa olmalı");

}  // namespace config
