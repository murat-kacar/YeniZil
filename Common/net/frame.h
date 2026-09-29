#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include "../kernel/byte_order.h"
#include "protocol.h"

namespace frame {                                         // Çerçeve biçimi v1: alanlar bayt bayt yazılır, struct kopyalanmaz

inline constexpr std::size_t kSize        = 20;           // Çerçeve boyutu (bayt)
inline constexpr std::size_t kHeaderSize  = 11;           // İmzalı, şifresiz başlık (bayt)
inline constexpr std::size_t kPayloadSize = 1;            // Şifreli ve imzalı içerik: mesaj tipi (bayt)
inline constexpr std::size_t kTagSize     = 8;            // AES-CCM doğrulama etiketi (bayt)
inline constexpr std::size_t kNonceSize   = 13;           // AES-CCM nonce (bayt)

inline constexpr std::size_t kVersionAt     = 0;          // sürüm (1)
inline constexpr std::size_t kApartmentAt   = 1;          // apartman kimliği (4)
inline constexpr std::size_t kSourceAt      = 5;          // kaynak (1)
inline constexpr std::size_t kDestinationAt = 6;          // hedef (1)
inline constexpr std::size_t kCounterAt     = 7;          // sayaç (4)
inline constexpr std::size_t kPayloadAt     = 11;         // mesaj tipi (1), şifreli
inline constexpr std::size_t kTagAt         = 12;         // doğrulama etiketi (8)

static_assert(kPayloadAt == kHeaderSize && kTagAt == kPayloadAt + kPayloadSize && kTagAt + kTagSize == kSize, "Alan yerleşimi çerçeve boyutuyla uyuşmuyor");

using Bytes = std::array<uint8_t, kSize>;                 // Ham çerçeve
using Nonce = std::array<uint8_t, kNonceSize>;            // AES-CCM nonce

struct Header {                                           // Başlık, tamamı imzalı
  uint8_t  version;                                       // Çerçeve biçimi sürümü
  uint32_t apartmentId;                                   // Apartman kimliği
  NodeId   source;                                        // Gönderen ünite
  NodeId   destination;                                   // Hedef ünite
  uint32_t counter;                                       // Gönderenin sayacı
};

constexpr Bytes encodeHeader(const Header& header) {      // Başlığı yazar, içerik ve etiket sıfır
  Bytes bytes{};
  const std::span<uint8_t, kSize> view(bytes);
  bytes[kVersionAt] = header.version;
  writeLe<uint32_t>(view.subspan<kApartmentAt, 4>(), header.apartmentId);
  bytes[kSourceAt]      = header.source;
  bytes[kDestinationAt] = header.destination;
  writeLe<uint32_t>(view.subspan<kCounterAt, 4>(), header.counter);
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

constexpr Nonce nonce(const Header& header) {             // apartmentId(4) | source(1) | counter(4) | version(1) | 0(3): (kaynak, sayaç) tekil olduğu için nonce da tekil
  Nonce bytes{};
  const std::span<uint8_t, kNonceSize> view(bytes);
  writeLe<uint32_t>(view.subspan<0, 4>(), header.apartmentId);
  bytes[4] = header.source;
  writeLe<uint32_t>(view.subspan<5, 4>(), header.counter);
  bytes[9] = header.version;
  return bytes;
}

constexpr std::span<const uint8_t, kHeaderSize> header(const Bytes& bytes) { return std::span<const uint8_t, kSize>(bytes).subspan<0, kHeaderSize>(); }  // İmzalı başlık
constexpr std::span<const uint8_t, kPayloadSize> payload(const Bytes& bytes) { return std::span<const uint8_t, kSize>(bytes).subspan<kPayloadAt, kPayloadSize>(); }  // Şifreli içerik
constexpr std::span<uint8_t, kPayloadSize> payload(Bytes& bytes) { return std::span<uint8_t, kSize>(bytes).subspan<kPayloadAt, kPayloadSize>(); }
constexpr std::span<const uint8_t, kTagSize> tag(const Bytes& bytes) { return std::span<const uint8_t, kSize>(bytes).subspan<kTagAt, kTagSize>(); }  // Doğrulama etiketi
constexpr std::span<uint8_t, kTagSize> tag(Bytes& bytes) { return std::span<uint8_t, kSize>(bytes).subspan<kTagAt, kTagSize>(); }

}  // namespace frame
