#pragma once

#include <cstddef>
#include "../../Common/io/press_detector.h"

namespace press_detector_tests {                          // PressDetector sınır testleri, derleme zamanında çalışır

inline constexpr PressConfig kConfig = {.minMs = 50, .maxMs = 30000, .cooldownMs = 3000};  // Test kuralları

struct Sample {                                           // Tek örnek
  bool     pressed;                                       // Buton basılı mı
  uint64_t atMs;                                          // Örnek zamanı (ms)
};

template <std::size_t N>
constexpr int acceptedCount(const Sample (&samples)[N]) {  // Senaryoda kabul edilen basış sayısı
  PressDetector detector;
  int count = 0;
  for (const Sample& sample : samples) count += detector.update(sample.pressed, sample.atMs, kConfig) ? 1 : 0;
  return count;
}

static_assert(acceptedCount({{false, 0}, {true, 100}, {false, 149}}) == 0, "49 ms parazit sayılmalı");
static_assert(acceptedCount({{false, 0}, {true, 100}, {false, 150}}) == 1, "50 ms kabul edilmeli");
static_assert(acceptedCount({{false, 0}, {true, 100}, {false, 30100}}) == 1, "30000 ms kabul edilmeli");
static_assert(acceptedCount({{false, 0}, {true, 100}, {false, 30101}}) == 0, "30001 ms kısa devre sayılmalı");
static_assert(acceptedCount({{true, 0}, {false, 200}}) == 0, "Açılışta basılı gelen buton sayılmamalı");
static_assert(acceptedCount({{true, 0}, {false, 200}, {true, 300}, {false, 400}}) == 1, "Açılıştan sonraki ilk gerçek basış kabul edilmeli");
static_assert(acceptedCount({{false, 0}, {true, 100}, {false, 200}, {true, 3000}, {false, 3199}}) == 1, "Bekleme süresi içindeki basış reddedilmeli");
static_assert(acceptedCount({{false, 0}, {true, 100}, {false, 200}, {true, 3000}, {false, 3200}}) == 2, "Bekleme süresi dolunca basış kabul edilmeli");
static_assert(acceptedCount({{false, 0}, {true, 100}, {false, 101}, {true, 102}, {false, 103}, {true, 104}, {false, 300}}) == 1, "Kontak sıçraması tek basış sayılmalı");
static_assert(acceptedCount({{false, 0}, {true, 100}, {false, 120}, {true, 200}, {false, 300}}) == 1, "Reddedilen basış bekleme süresini başlatmamalı");
static_assert(acceptedCount({{false, 0}, {true, 100}, {true, 5000}, {false, 5100}}) == 1, "Basılı tutarken gelen örnekler basışı bölmemeli");

}  // namespace press_detector_tests
