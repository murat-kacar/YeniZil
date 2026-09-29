#pragma once

#include <stdint.h>

namespace config {                                        // Ürün ayarları

inline constexpr uint32_t kHeartbeatIntervalMs = 30000;   // Dış ünitenin "buradayım" yayın aralığı (ms)
inline constexpr uint32_t kLinkTimeoutMs       = 3 * kHeartbeatIntervalMs + 5000;  // Bu süre sinyal gelmezse bağlantı koptu sayılır (ms): 3 kaçırılan yayın + pay

static_assert(kHeartbeatIntervalMs >= 10000, "Yayın aralığı en az 10 sn olmalı: her yayın tüm üniteleri 420 ms gönderime sokar");
static_assert(kLinkTimeoutMs > 2 * kHeartbeatIntervalMs, "Tek kaçırılan yayın bağlantıyı düşürmemeli");

}  // namespace config
