#pragma once

#include "../../Common/net/frame.h"

namespace frame_tests {                                   // Çerçeve biçimi, derleme zamanında çalışır

inline constexpr frame::Header kHeader = {.version = 1, .apartmentId = 0xA1B2C3D4, .source = 3, .destination = 0, .counter = 0x01020304};  // Örnek başlık

inline constexpr frame::Bytes kEncoded = frame::encode(kHeader, static_cast<uint8_t>(MessageType::kOpenDoor));  // Örnek çerçeve

constexpr bool roundTrip() {                              // Yazılan başlık aynen okunur
  const frame::Header header = frame::decodeHeader(kEncoded);
  return header.version == kHeader.version && header.apartmentId == kHeader.apartmentId && header.source == kHeader.source &&
         header.destination == kHeader.destination && header.counter == kHeader.counter;
}

static_assert(roundTrip(), "Başlık gidiş-dönüş aynı olmalı");
static_assert(frame::typeByte(kEncoded) == static_cast<uint8_t>(MessageType::kOpenDoor), "Tip baytı korunmalı");
static_assert(kEncoded[frame::kApartmentAt] == 0xD4 && kEncoded[frame::kApartmentAt + 3] == 0xA1, "Apartman kimliği little-endian yazılmalı");
static_assert(kEncoded[frame::kCounterAt] == 0x04 && kEncoded[frame::kCounterAt + 3] == 0x01, "Sayaç little-endian yazılmalı");
static_assert(kEncoded[frame::kTagAt] == 0 && kEncoded[frame::kSize - 1] == 0, "Etiket Aşama 3'e kadar sıfır olmalı");
static_assert(isKnownMessageType(1) && isKnownMessageType(2) && !isKnownMessageType(0) && !isKnownMessageType(3), "Tip doğrulaması");

}  // namespace frame_tests
