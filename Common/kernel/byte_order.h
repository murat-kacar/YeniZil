#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <span>

static_assert(std::endian::native == std::endian::little, "Çerçeve little-endian: ESP32-C3 (RISC-V) little-endian");

template <std::unsigned_integral T>
constexpr void writeLe(std::span<uint8_t, sizeof(T)> out, T value) {  // Tamsayıyı little-endian yazar
  std::ranges::copy(std::bit_cast<std::array<uint8_t, sizeof(T)>>(value), out.begin());
}

template <std::unsigned_integral T>
constexpr T readLe(std::span<const uint8_t, sizeof(T)> in) {          // Little-endian tamsayıyı okur
  std::array<uint8_t, sizeof(T)> bytes{};
  std::ranges::copy(in, bytes.begin());
  return std::bit_cast<T>(bytes);
}
