#pragma once

#include <Arduino.h>
#include "component.h"

class DebugSerial : public Component {                                        // Core loglarını USB seri porta yönlendirir, sadece log seviyesi açıksa çalışır
 public:
  static constexpr uint32_t kBaud       = 115200;                             // Seri hız
  static constexpr uint32_t kHostWaitMs = 3000;                               // Açılışta seri monitörün bağlanmasını bekleme süresi (ms), açılış logları kaybolmasın

  void begin() override {
    if constexpr (kEnabled) {
      Serial.begin(kBaud);
      Serial.setDebugOutput(true);                                            // log_i/log_e çıktısı UART0 yerine USB'ye gider
      const uint32_t startMs = millis();
      while (!Serial && millis() - startMs < kHostWaitMs) delay(10);
    }
  }

  void update(uint64_t) override {}

 private:
  static constexpr bool kEnabled = ARDUHAL_LOG_LEVEL > ARDUHAL_LOG_LEVEL_NONE;  // Arduino IDE "Core Debug Level" menüsünden gelir, None ise hiçbir şey yapmaz
};
