#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include "protocol.h"

template <std::size_t N>
constexpr std::optional<NodeId> findNodeId(const MacAddress (&table)[N], const MacAddress& mac) {  // MAC'in tablodaki sırası = ünite kimliği
  const auto found = std::find(std::begin(table), std::end(table), mac);
  if (found == std::end(table)) return std::nullopt;
  return static_cast<NodeId>(std::distance(std::begin(table), found));
}
