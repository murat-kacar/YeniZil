#pragma once

#include <cstddef>
#include <optional>
#include "protocol.h"

template <std::size_t N>
constexpr std::optional<NodeId> findNodeId(const MacAddress (&table)[N], const MacAddress& mac) {  // MAC'in tablodaki sırası = ünite kimliği
  for (std::size_t i = 0; i < N; ++i)
    if (table[i] == mac) return static_cast<NodeId>(i);
  return std::nullopt;
}
