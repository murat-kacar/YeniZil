#pragma once

#include <Arduino.h>
#include <ESP32_NOW.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include "../kernel/clock.h"
#include "../kernel/component.h"
#include "../kernel/event_loop.h"
#include "broadcast_peer.h"
#include "protocol.h"

struct RadioConfig {                                                    // Radyo ayarları
  uint8_t  channel;                                                     // Wi-Fi kanalı (1-13), tüm ünitelerde aynı
  int8_t   txPowerDbm;                                                  // Gönderim gücü (dBm), 2-20
  bool     longRange;                                                   // Espressif Long Range modu, tüm ünitelerde aynı
  uint16_t burstPeriodMs;                                               // Bir çerçevenin kopyaları arasındaki süre (ms)
  uint16_t burstDurationMs;                                             // Bir çerçevenin tekrarlanma süresi (ms)
  uint16_t relayJitterMaxMs;                                            // Aktarmadan önceki en fazla rastgele bekleme (ms)
  uint16_t wakeIntervalMs;                                              // Radyonun uyanma aralığı (ms), tüm ünitelerde aynı
  uint16_t wakeWindowMs;                                                // Her aralıkta dinlenen süre (ms), arası modem uykusu
};

struct ReceivedFrame {                                                  // Alınan çerçeve
  static constexpr std::size_t kMaxBytes = 32;                          // En büyük çerçeve (bayt), protokol çerçevesi 20 bayt

  uint8_t                        length = 0;                            // Veri uzunluğu (bayt)
  std::array<uint8_t, kMaxBytes> bytes{};                               // Veri
};

class EspNowRadio : public Component {                                  // ESP-NOW radyo: uyanma penceresinde dinler, çerçeveyi süre boyunca tekrarlayarak yayınlar, alınanları kuyruğa koyar
 public:
  static constexpr std::size_t kQueueLength = 8;                        // Alma kuyruğu uzunluğu (çerçeve)
  static constexpr std::size_t kMaxBursts   = 4;                        // Aynı anda tekrarlanan en fazla çerçeve

  explicit EspNowRadio(const RadioConfig& config) : config_(config), peer_(config.longRange) {}

  void begin() override {
    queue_ = xQueueCreateStatic(kQueueLength, sizeof(ReceivedFrame), queueStorage_.data(), &queueControl_);
    WiFi.enableLongRange(config_.longRange);                            // mode()'dan önce verilmeli
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(WIFI_PS_MIN_MODEM);                                   // Modem uykusu: radyo sadece uyanma penceresinde açık
    WiFi.setChannel(config_.channel);
    WiFi.setTxPower(static_cast<wifi_power_t>(config_.txPowerDbm * 4)); // Sürücü 0,25 dBm birimi kullanır
    if (!ESP_NOW.begin()) return;
    ESP_NOW.onNewPeer(&EspNowRadio::onReceive, this);                   // Göndericiler eş olarak eklenmez, tüm çerçeveler buraya gelir
    if (!peer_.begin()) return;
    esp_now_set_wake_window(config_.wakeWindowMs);                      // Her aralıkta bu kadar dinler (Arduino karşılığı yok)
    esp_wifi_connectionless_module_set_wake_interval(config_.wakeIntervalMs);  // Uyanma aralığı (Arduino karşılığı yok)
  }

  void update(uint64_t nowMs) override {                                // Zamanı gelen kopyaları gönderir
    for (Burst& burst : bursts_) {
      if (!burst.active || nowMs < burst.nextSendMs) continue;
      peer_.send(std::span<const uint8_t>(burst.bytes.data(), burst.length));
      burst.nextSendMs = nowMs + config_.burstPeriodMs;
      if (burst.nextSendMs > burst.endMs) burst.active = false;
    }
  }

  uint64_t nextDeadlineMs() const override {
    uint64_t deadlineMs = kNoDeadlineMs;
    for (const Burst& burst : bursts_)
      if (burst.active) deadlineMs = std::min(deadlineMs, burst.nextSendMs);
    return deadlineMs;
  }

  bool broadcast(std::span<const uint8_t> bytes) { return startBurst(bytes, 0); }  // Kendi çerçevesini hemen yayınlamaya başlar

  bool relay(std::span<const uint8_t> bytes) {                          // Başkasının çerçevesini rastgele kısa bir beklemeden sonra aktarır
    return startBurst(bytes, static_cast<uint32_t>(random(config_.relayJitterMaxMs + 1)));  // Arduino random(): Wi-Fi açıkken donanım RNG
  }

  [[nodiscard]] bool receive(ReceivedFrame& frame) {                    // Kuyruktaki sıradaki çerçeveyi alır, yoksa false
    return queue_ != nullptr && xQueueReceive(queue_, &frame, 0) == pdTRUE;
  }

  static MacAddress ownMac() {                                          // Bu kartın MAC adresi
    MacAddress mac{};
    WiFi.macAddress(mac.data());
    return mac;
  }

 private:
  struct Burst {                                                        // Tekrarlanan bir çerçeve
    std::array<uint8_t, ReceivedFrame::kMaxBytes> bytes{};              // Çerçeve
    uint8_t  length     = 0;                                            // Çerçeve uzunluğu
    uint64_t nextSendMs = 0;                                            // Sıradaki kopyanın zamanı
    uint64_t endMs      = 0;                                            // Tekrarın bittiği an
    bool     active     = false;                                        // Tekrarlanıyor mu
  };

  bool startBurst(std::span<const uint8_t> bytes, uint32_t delayMs) {  // Boş bir yuvada tekrarı başlatır
    if (bytes.size() > ReceivedFrame::kMaxBytes) return false;
    for (Burst& burst : bursts_) {
      if (burst.active) continue;
      std::copy(bytes.begin(), bytes.end(), burst.bytes.begin());
      burst.length     = static_cast<uint8_t>(bytes.size());
      burst.nextSendMs = monotonicMs() + delayMs;
      burst.endMs      = burst.nextSendMs + config_.burstDurationMs;
      burst.active     = true;
      return true;
    }
    return false;                                                       // Tüm yuvalar dolu
  }

  static void onReceive(const esp_now_recv_info_t*, const uint8_t* data, int length, void* arg) {  // Wi-Fi görevinde çalışır: kopyalar, kuyruğa koyar, döngüyü uyandırır
    if (length <= 0 || static_cast<std::size_t>(length) > ReceivedFrame::kMaxBytes) return;
    ReceivedFrame frame;
    frame.length = static_cast<uint8_t>(length);
    std::copy_n(data, length, frame.bytes.begin());
    xQueueSend(static_cast<EspNowRadio*>(arg)->queue_, &frame, 0);
    eventLoop.notify();
  }

  RadioConfig                     config_;                              // Radyo ayarları
  BroadcastPeer                   peer_;                                // Yayın eşi
  std::array<Burst, kMaxBursts>   bursts_{};                            // Tekrarlanan çerçeveler
  QueueHandle_t                   queue_ = nullptr;                     // Alma kuyruğu
  StaticQueue_t                   queueControl_{};                      // Kuyruk denetim bloğu, heap kullanılmaz
  std::array<uint8_t, kQueueLength * sizeof(ReceivedFrame)> queueStorage_{};  // Kuyruk belleği
};
