#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include "../kernel/byte_order.h"
#include "protocol.h"

namespace frame {                                         // Çerçeve biçimi v1: alanlar bayt bayt yazılır, struct kopyalanmaz

inline constexpr std::size_t kSize    = 20;               // Çerçeve boyutu (bayt)
inline constexpr std::size_t kTagSize = 8;                // Doğrulama etiketi (bayt), Aşama 3'te AES-CCM

inline constexpr std::size_t kVersionAt     = 0;          // sürüm (1)
inline constexpr std::size_t kApartmentAt   = 1;          // apartman kimliği (4)
inline constexpr std::size_t kSourceAt      = 5;          // kaynak (1)
inline constexpr std::size_t kDestinationAt = 6;          // hedef (1)
inline constexpr std::size_t kCounterAt     = 7;          // sayaç (4)
inline constexpr std::size_t kTypeAt        = 11;         // mesaj tipi (1), Aşama 3'te şifreli
inline constexpr std::size_t kTagAt         = 12;         // doğrulama etiketi (8)

static_assert(kTagAt + kTagSize == kSize, "Alan yerleşimi çerçeve boyutuyla uyuşmuyor");

using Bytes = std::array<uint8_t, kSize>;                 // Ham çerçeve

struct Header {                                           // Başlık, Aşama 3'te tamamı imzalı
  uint8_t  version;                                       // Çerçeve biçimi sürümü
  uint32_t apartmentId;                                   // Apartman kimliği
  NodeId   source;                                        // Gönderen ünite
  NodeId   destination;                                   // Hedef ünite
  uint32_t counter;                                       // Gönderenin sayacı
};

constexpr Bytes encode(const Header& header, uint8_t typeByte) {  // Başlık ve tipten çerçeve üretir, etiket sıfır
  Bytes bytes{};
  const std::span<uint8_t, kSize> view(bytes);
  bytes[kVersionAt] = header.version;
  writeLe<uint32_t>(view.subspan<kApartmentAt, 4>(), header.apartmentId);
  bytes[kSourceAt]      = header.source;
  bytes[kDestinationAt] = header.destination;
  writeLe<uint32_t>(view.subspan<kCounterAt, 4>(), header.counter);
  bytes[kTypeAt] = typeByte;
  return bytes;
}

constexpr Header decodeHeader(const Bytes& bytes) {       // Çerçeveden başlığı okur
  const std::span<const uint8_t, kSize> view(bytes);
  return {
      .version     = bytes[kVersionAt],
      .apartmentId = readLe<uint32_t>(view.subspan<kApartmentAt, 4>()),
      .source      = bytes[kSourceAt],
      .destination = bytes[kDestinationAt],
      .counter     = readLe<uint32_t>(view.subspan<kCounterAt, 4>()),
  };
}

constexpr uint8_t typeByte(const Bytes& bytes) { return bytes[kTypeAt]; }  // Mesaj tipi baytı

}  // namespace frame
