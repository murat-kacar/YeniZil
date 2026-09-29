#pragma once

#include <Arduino.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include "../config/radio_settings.h"
#include "../kernel/clock.h"
#include "../kernel/component.h"
#include "esp_now_radio.h"

namespace yenizil {

class BurstSender : public Component {                                  // Çerçeveyi süre boyunca aralıklarla tekrar gönderir: uyku penceresindeki alıcılar yakalar
 public:
  static constexpr std::size_t kMaxBursts = 4;                          // Aynı anda tekrarlanan en fazla çerçeve

  BurstSender(EspNowRadio& radio, const BurstSettings& settings) : radio_(radio), settings_(settings) {}

  void update(uint64_t nowMs) override {                                // Zamanı gelen kopyaları gönderir
    for (Burst& burst : bursts_) {
      if (!burst.active || nowMs < burst.nextSendMs) continue;
      radio_.send(std::span<const uint8_t>(burst.bytes.data(), burst.length));
      burst.nextSendMs = nowMs + settings_.periodMs;
      if (burst.nextSendMs > burst.endMs) burst.active = false;
    }
  }

  uint64_t nextDeadlineMs() const override {
    uint64_t deadlineMs = kNoDeadlineMs;
    for (const Burst& burst : bursts_)
      if (burst.active) deadlineMs = std::min(deadlineMs, burst.nextSendMs);
    return deadlineMs;
  }

  bool send(std::span<const uint8_t> bytes) { return start(bytes, 0); }  // Kendi çerçevesini hemen tekrarlamaya başlar

  bool relay(std::span<const uint8_t> bytes) {                          // Başkasının çerçevesini rastgele kısa bir beklemeden sonra tekrarlar
    return start(bytes, static_cast<uint32_t>(random(settings_.relayJitterMaxMs + 1)));  // Arduino random(): Wi-Fi açıkken donanım RNG
  }

 private:
  struct Burst {                                                        // Tekrarlanan bir çerçeve
    std::array<uint8_t, ReceivedFrame::kMaxBytes> bytes{};              // Çerçeve
    uint8_t  length     = 0;                                            // Çerçeve uzunluğu
    uint64_t nextSendMs = 0;                                            // Sıradaki kopyanın zamanı
    uint64_t endMs      = 0;                                            // Tekrarın bittiği an
    bool     active     = false;                                        // Tekrarlanıyor mu
  };

  bool start(std::span<const uint8_t> bytes, uint32_t delayMs) {       // Boş bir yuvada tekrarı başlatır
    if (bytes.size() > ReceivedFrame::kMaxBytes) return false;
    const auto slot = std::find_if(bursts_.begin(), bursts_.end(), [](const Burst& burst) { return !burst.active; });
    if (slot == bursts_.end()) return false;                            // Tüm yuvalar dolu
    std::ranges::copy(bytes, slot->bytes.begin());
    slot->length     = static_cast<uint8_t>(bytes.size());
    slot->nextSendMs = monotonicMs() + delayMs;
    slot->endMs      = slot->nextSendMs + settings_.durationMs;
    slot->active     = true;
    return true;
  }

  EspNowRadio&                  radio_;                                 // Kopyaları gönderen radyo
  BurstSettings                 settings_;                              // Tekrar ayarları
  std::array<Burst, kMaxBursts> bursts_{};                              // Tekrarlanan çerçeveler
};

}  // namespace yenizil
