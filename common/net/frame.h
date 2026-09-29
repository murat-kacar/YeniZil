#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include "../kernel/byte_order.h"
#include "protocol.h"

namespace frame {                                         // Çerçeve biçimi v2: alanlar bayt bayt yazılır, struct kopyalanmaz

inline constexpr std::size_t kSize        = 26;           // Çerçeve boyutu (bayt)
inline constexpr std::size_t kHeaderSize  = 17;           // İmzalı, şifresiz başlık (bayt)
inline constexpr std::size_t kPayloadSize = 1;            // Şifreli ve imzalı içerik: mesaj tipi (bayt)
inline constexpr std::size_t kTagSize     = 8;            // AES-CCM doğrulama etiketi (bayt)
inline constexpr std::size_t kNonceSize   = 13;           // AES-CCM nonce (bayt)

inline constexpr std::size_t kVersionAt     = 0;          // sürüm (1)
inline constexpr std::size_t kApartmentAt   = 1;          // apartman kimliği (4)
inline constexpr std::size_t kSourceMacAt   = 5;          // gönderen kartın MAC adresi (6)
inline constexpr std::size_t kSourceAt      = 11;         // kaynak (1)
inline constexpr std::size_t kDestinationAt = 12;         // hedef (1)
inline constexpr std::size_t kCounterAt     = 13;         // sayaç (4)
inline constexpr std::size_t kPayloadAt     = 17;         // mesaj tipi (1), şifreli
inline constexpr std::size_t kTagAt         = 18;         // doğrulama etiketi (8)

static_assert(kPayloadAt == kHeaderSize && kTagAt == kPayloadAt + kPayloadSize && kTagAt + kTagSize == kSize, "Alan yerleşimi çerçeve boyutuyla uyuşmuyor");

using Bytes = std::array<uint8_t, kSize>;                 // Ham çerçeve
using Nonce = std::array<uint8_t, kNonceSize>;            // AES-CCM nonce

struct Header {                                           // Başlık, tamamı imzalı
  uint8_t    version;                                     // Çerçeve biçimi sürümü
  uint32_t   apartmentId;                                 // Apartman kimliği
  MacAddress sourceMac;                                   // Gönderen kartın MAC adresi: nonce'u kimlikten bağımsız tekil yapar
  NodeId     source;                                      // Gönderen ünite
  NodeId     destination;                                 // Hedef ünite
  uint32_t   counter;                                     // Gönderenin sayacı
};

constexpr Bytes encodeHeader(const Header& header) {      // Başlığı yazar, içerik ve etiket sıfır
  Bytes bytes{};
  const std::span<uint8_t, kSize> view(bytes);
  bytes[kVersionAt] = header.version;
  writeLe<uint32_t>(view.subspan<kApartmentAt, 4>(), header.apartmentId);
  std::ranges::copy(header.sourceMac, view.subspan<kSourceMacAt, 6>().begin());
  bytes[kSourceAt]      = header.source;
  bytes[kDestinationAt] = header.destination;
  writeLe<uint32_t>(view.subspan<kCounterAt, 4>(), header.counter);
  return bytes;
}

constexpr Header decodeHeader(const Bytes& bytes) {       // Çerçeveden başlığı okur
  const std::span<const uint8_t, kSize> view(bytes);
  Header header{
      .version     = bytes[kVersionAt],
      .apartmentId = readLe<uint32_t>(view.subspan<kApartmentAt, 4>()),
      .sourceMac   = {},
      .source      = bytes[kSourceAt],
      .destination = bytes[kDestinationAt],
      .counter     = readLe<uint32_t>(view.subspan<kCounterAt, 4>()),
  };
  std::ranges::copy(view.subspan<kSourceMacAt, 6>(), header.sourceMac.begin());
  return header;
}

constexpr Nonce nonce(const Header& header) {             // sourceMac(6) | counter(4) | version(1) | 0(2): MAC fabrikadan tekil, sayaç kalıcı; kimlikler çakışsa da nonce tekrarlanmaz
  Nonce bytes{};
  const std::span<uint8_t, kNonceSize> view(bytes);
  std::ranges::copy(header.sourceMac, view.begin());
  writeLe<uint32_t>(view.subspan<6, 4>(), header.counter);
  bytes[10] = header.version;
  return bytes;
}

constexpr std::span<const uint8_t, kHeaderSize> header(const Bytes& bytes) { return std::span<const uint8_t, kSize>(bytes).subspan<0, kHeaderSize>(); }  // İmzalı başlık
constexpr std::span<const uint8_t, kPayloadSize> payload(const Bytes& bytes) { return std::span<const uint8_t, kSize>(bytes).subspan<kPayloadAt, kPayloadSize>(); }  // Şifreli içerik
constexpr std::span<uint8_t, kPayloadSize> payload(Bytes& bytes) { return std::span<uint8_t, kSize>(bytes).subspan<kPayloadAt, kPayloadSize>(); }
constexpr std::span<const uint8_t, kTagSize> tag(const Bytes& bytes) { return std::span<const uint8_t, kSize>(bytes).subspan<kTagAt, kTagSize>(); }  // Doğrulama etiketi
constexpr std::span<uint8_t, kTagSize> tag(Bytes& bytes) { return std::span<uint8_t, kSize>(bytes).subspan<kTagAt, kTagSize>(); }

}  // namespace frame
