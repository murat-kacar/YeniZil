#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

template <std::unsigned_integral T>
constexpr void writeLe(std::span<uint8_t, sizeof(T)> out, T value) {  // Tamsayıyı little-endian yazar
  for (std::size_t i = 0; i < sizeof(T); ++i) out[i] = static_cast<uint8_t>(value >> (8 * i));
}

template <std::unsigned_integral T>
constexpr T readLe(std::span<const uint8_t, sizeof(T)> in) {          // Little-endian tamsayıyı okur
  T value = 0;
  for (std::size_t i = 0; i < sizeof(T); ++i) value |= static_cast<T>(static_cast<T>(in[i]) << (8 * i));
  return value;
}
