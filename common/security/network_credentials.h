#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include "../net/protocol.h"
#include "ccm_cipher.h"

namespace yenizil {

struct NetworkCredentials {                               // Bir ağın kimliği ve şifresi: ünite config dosyasından gelir
  NetworkId                                     id;       // Ağ kimliği, çerçeve başlığında
  std::span<const uint8_t, CcmCipher::kKeySize> key;      // Ağ şifresi (AES-128)
};

constexpr bool isAssigned(const NetworkCredentials& network) {  // Kimlik ve şifre doldurulmuş mu (sıfır değil)
  return network.id != NetworkId{0} && std::ranges::any_of(network.key, [](uint8_t byte) { return byte != 0; });
}

constexpr bool isSeparate(const NetworkCredentials& first, const NetworkCredentials& second) {  // İki ağın kimliği de şifresi de farklı mı
  return first.id != second.id && !std::ranges::equal(first.key, second.key);
}

}  // namespace yenizil
