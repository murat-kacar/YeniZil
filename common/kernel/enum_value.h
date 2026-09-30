#pragma once

#include <type_traits>

namespace yenizil {

template <typename E>
  requires std::is_enum_v<E>
constexpr std::underlying_type_t<E> toUnderlying(E value) {  // Enum'un taşıdığı sayı: C++23 std::to_underlying karşılığı
  return static_cast<std::underlying_type_t<E>>(value);
}

}  // namespace yenizil
