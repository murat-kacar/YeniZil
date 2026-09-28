#pragma once

#include "../../OutDoor/outdoor_unit.h"
#include "../../Common/io/button.h"
#include "../../Common/app/bell.h"

namespace bench {                                         // Tezgâh testi: dış ünite kablolaması + tek buton ve zil

inline constexpr uint8_t kSingleButton = 7;               // Tek buton testi: pin - buton - GND
inline constexpr uint8_t kBellOutput   = 0;               // Zil yerine LED: pin - 330Ω - LED - GND

inline constexpr PressConfig kSinglePress = {             // Tek buton kuralları, iç ünite ile aynı
    .minMs      = config::kMinPressMs,
    .maxMs      = config::kMaxPressMs,
    .cooldownMs = 2000,
};

inline Button singleButton(kSingleButton, LOW, kSinglePress);  // Tek buton
inline Bell   bell(kBellOutput, HIGH, 1500);                   // Zil yerine LED

inline void reportFlatPress(uint8_t flat) {               // Daire butonu: log + kapı çıkışı
  Serial.printf("Daire %u butonu, bos heap %lu\n", flat, static_cast<unsigned long>(ESP.getFreeHeap()));
  doorOpener.open();
}

inline void reportSinglePress() {                         // Tek buton: log + zil çıkışı
  Serial.printf("Tek buton, bos heap %lu\n", static_cast<unsigned long>(ESP.getFreeHeap()));
  bell.ring();
}

}  // namespace bench
