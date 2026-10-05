#pragma once

#include <Arduino.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include "../config/radio_settings.h"
#include "../kernel/clock.h"
#include "../kernel/component.h"
#include "esp_now_radio.h"
#include "frame.h"

namespace yenizil {

class BurstSender : public Component {                                  // Çerçeveyi birkaç kez aralıklarla gönderir: onaysız yayında tek bir kayıp mesajı düşürmez
 public:
  static constexpr std::size_t kMaxBursts = 4;                          // Aynı anda tekrarlanan en fazla çerçeve

  BurstSender(EspNowRadio& radio, const BurstSettings& settings) : radio_(radio), settings_(settings) {}

  void update(uint64_t nowMs) override {                                // Zamanı gelen kopyaları gönderir
    for (Burst& burst : bursts_) {
      if (burst.remaining == 0 || nowMs < burst.nextSendMs) continue;
      static_cast<void>(radio_.send(burst.bytes));                     // Tek kopyanın kaybı sorun değil: kalan kopyalar gider
      burst.nextSendMs = nowMs + settings_.intervalMs;
      --burst.remaining;
    }
  }

  uint64_t nextDeadlineMs() const override {
    uint64_t deadlineMs = kNoDeadlineMs;
    for (const Burst& burst : bursts_)
      if (burst.remaining > 0) deadlineMs = std::min(deadlineMs, burst.nextSendMs);
    return deadlineMs;
  }

  [[nodiscard]] bool send(const frame::Bytes& bytes) { return start(bytes, 0); }      // Kendi çerçevesini hemen tekrarlamaya başlar

  [[nodiscard]] bool relay(const frame::Bytes& bytes) {                               // Başkasının çerçevesini rastgele kısa bir beklemeden sonra tekrarlar
    return start(bytes, static_cast<uint32_t>(random(settings_.relayJitterMaxMs + 1)));  // Arduino random(): Wi-Fi açıkken donanım RNG
  }

 private:
  struct Burst {                                                        // Tekrarlanan bir çerçeve
    frame::Bytes bytes{};                                               // Çerçeve
    uint64_t     nextSendMs = 0;                                        // Sıradaki kopyanın zamanı
    uint8_t      remaining  = 0;                                        // Gönderilecek kopya sayısı, 0: yuva boş
  };

  bool start(const frame::Bytes& bytes, uint32_t delayMs) {            // Boş bir yuvada tekrarı başlatır
    const auto slot = std::find_if(bursts_.begin(), bursts_.end(), [](const Burst& burst) { return burst.remaining == 0; });
    if (slot == bursts_.end()) return false;                            // Tüm yuvalar dolu
    slot->bytes      = bytes;
    slot->nextSendMs = monotonicMs() + delayMs;
    slot->remaining  = settings_.copies;
    return true;
  }

  EspNowRadio&                  radio_;                                 // Kopyaları gönderen radyo
  BurstSettings                 settings_;                              // Tekrar ayarları
  std::array<Burst, kMaxBursts> bursts_{};                              // Tekrarlanan çerçeveler
};

}  // namespace yenizil
