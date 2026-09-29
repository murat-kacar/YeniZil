#pragma once

#include "../../Common/net/node_identity.h"

namespace node_identity_tests {                           // MAC tablosundan kimlik bulma, derleme zamanında çalışır

inline constexpr MacAddress kTable[] = {                  // Örnek tablo
    {0x34, 0x85, 0x18, 0x00, 0x00, 0x10},
    {0x34, 0x85, 0x18, 0x00, 0x00, 0x11},
    {0x34, 0x85, 0x18, 0x00, 0x00, 0x12},
};

static_assert(findNodeId(kTable, {0x34, 0x85, 0x18, 0x00, 0x00, 0x10}) == NodeId{0}, "İlk satır dış ünite (0) olmalı");
static_assert(findNodeId(kTable, {0x34, 0x85, 0x18, 0x00, 0x00, 0x12}) == NodeId{2}, "Üçüncü satır kimlik 2 olmalı");
static_assert(!findNodeId(kTable, {0x34, 0x85, 0x18, 0x00, 0x00, 0x13}).has_value(), "Tabloda olmayan MAC bulunmamalı");

}  // namespace node_identity_tests
