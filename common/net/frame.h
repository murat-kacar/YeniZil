#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include "../kernel/byte_order.h"
#include "../kernel/enum_value.h"
#include "protocol.h"

namespace yenizil::frame {                                // Çerçeve biçimi v2: alanlar bayt bayt yazılır, struct kopyalanmaz

inline constexpr std::size_t kMacSize       = std::tuple_size_v<MacAddress>;  // MAC adresi (bayt)
inline constexpr std::size_t kNetworkSize   = sizeof(NetworkId);              // Ağ kimliği (bayt)
inline constexpr std::size_t kCounterSize   = sizeof(FrameCounter);           // Sayaç (bayt)
inline constexpr std::size_t kSize          = 26;         // Çerçeve boyutu (bayt)
inline constexpr std::size_t kHeaderSize    = 17;         // İmzalı, şifresiz başlık (bayt)
inline constexpr std::size_t kPayloadSize   = 1;          // Şifreli ve imzalı içerik: mesaj tipi (bayt)
inline constexpr std::size_t kTagSize       = 8;          // AES-CCM doğrulama etiketi (bayt)
inline constexpr std::size_t kNonceSize     = 13;         // AES-CCM nonce (bayt)

inline constexpr std::size_t kVersionAt     = 0;          // sürüm (1)
inline constexpr std::size_t kNetworkAt     = 1;          // ağ kimliği (4)
inline constexpr std::size_t kSourceMacAt   = 5;          // gönderen kartın MAC adresi (6)
inline constexpr std::size_t kSourceAt      = 11;         // kaynak (1)
inline constexpr std::size_t kDestinationAt = 12;         // hedef (1)
inline constexpr std::size_t kCounterAt     = 13;         // sayaç (4)
inline constexpr std::size_t kPayloadAt     = 17;         // mesaj tipi (1), şifreli
inline constexpr std::size_t kTagAt         = 18;         // doğrulama etiketi (8)

inline constexpr std::size_t kNonceMacAt     = 0;         // nonce: gönderen MAC (6)
inline constexpr std::size_t kNonceCounterAt = 6;         // nonce: sayaç (4)
inline constexpr std::size_t kNonceVersionAt = 10;        // nonce: sürüm (1), kalan 2 bayt sıfır

static_assert(kSourceMacAt == kNetworkAt + kNetworkSize && kSourceAt == kSourceMacAt + kMacSize && kPayloadAt == kCounterAt + kCounterSize,
              "Alan yerleşimi alan boyutlarıyla uyuşmuyor");
static_assert(kPayloadAt == kHeaderSize && kTagAt == kPayloadAt + kPayloadSize && kTagAt + kTagSize == kSize, "Alan yerleşimi çerçeve boyutuyla uyuşmuyor");
static_assert(kNonceCounterAt == kNonceMacAt + kMacSize && kNonceVersionAt == kNonceCounterAt + kCounterSize && kNonceVersionAt < kNonceSize,
              "Nonce yerleşimi nonce boyutuyla uyuşmuyor");

using Bytes = std::array<uint8_t, kSize>;                 // Ham çerçeve
using Nonce = std::array<uint8_t, kNonceSize>;            // AES-CCM nonce

struct Header {                                           // Başlık, tamamı imzalı
  uint8_t      version;                                   // Çerçeve biçimi sürümü
  NetworkId    networkId;                                 // Ağ kimliği
  MacAddress   sourceMac;                                 // Gönderen kartın MAC adresi: nonce'u kimlikten bağımsız tekil yapar
  NodeId       source;                                    // Gönderen ünite
  NodeId       destination;                               // Hedef ünite
  FrameCounter counter;                                   // Gönderenin sayacı
};

constexpr Bytes encodeHeader(const Header& header) {      // Başlığı yazar, içerik ve etiket sıfır
  Bytes bytes{};
  const std::span<uint8_t, kSize> view(bytes);
  bytes[kVersionAt] = header.version;
  writeLe(view.subspan<kNetworkAt, kNetworkSize>(), toUnderlying(header.networkId));
  std::ranges::copy(header.sourceMac, view.subspan<kSourceMacAt, kMacSize>().begin());
  bytes[kSourceAt]      = toUnderlying(header.source);
  bytes[kDestinationAt] = toUnderlying(header.destination);
  writeLe(view.subspan<kCounterAt, kCounterSize>(), toUnderlying(header.counter));
  return bytes;
}

constexpr Header decodeHeader(const Bytes& bytes) {       // Çerçeveden başlığı okur
  const std::span<const uint8_t, kSize> view(bytes);
  Header header{
      .version     = bytes[kVersionAt],
      .networkId   = NetworkId{readLe<uint32_t>(view.subspan<kNetworkAt, kNetworkSize>())},
      .sourceMac   = {},
      .source      = NodeId{bytes[kSourceAt]},
      .destination = NodeId{bytes[kDestinationAt]},
      .counter     = FrameCounter{readLe<uint32_t>(view.subspan<kCounterAt, kCounterSize>())},
  };
  std::ranges::copy(view.subspan<kSourceMacAt, kMacSize>(), header.sourceMac.begin());
  return header;
}

constexpr Nonce nonce(const Header& header) {             // sourceMac | counter | version | 0: MAC fabrikadan tekil, sayaç kalıcı; kimlikler çakışsa da nonce tekrarlanmaz (IEEE 802.15.4 CCM*)
  Nonce bytes{};
  const std::span<uint8_t, kNonceSize> view(bytes);
  std::ranges::copy(header.sourceMac, view.subspan<kNonceMacAt, kMacSize>().begin());
  writeLe(view.subspan<kNonceCounterAt, kCounterSize>(), toUnderlying(header.counter));
  bytes[kNonceVersionAt] = header.version;
  return bytes;
}

constexpr std::span<const uint8_t, kHeaderSize> header(const Bytes& bytes) { return std::span<const uint8_t, kSize>(bytes).subspan<0, kHeaderSize>(); }  // İmzalı başlık
constexpr std::span<const uint8_t, kPayloadSize> payload(const Bytes& bytes) { return std::span<const uint8_t, kSize>(bytes).subspan<kPayloadAt, kPayloadSize>(); }  // Şifreli içerik
constexpr std::span<uint8_t, kPayloadSize> payload(Bytes& bytes) { return std::span<uint8_t, kSize>(bytes).subspan<kPayloadAt, kPayloadSize>(); }
constexpr std::span<const uint8_t, kTagSize> tag(const Bytes& bytes) { return std::span<const uint8_t, kSize>(bytes).subspan<kTagAt, kTagSize>(); }  // Doğrulama etiketi
constexpr std::span<uint8_t, kTagSize> tag(Bytes& bytes) { return std::span<uint8_t, kSize>(bytes).subspan<kTagAt, kTagSize>(); }

}  // namespace yenizil::frame
