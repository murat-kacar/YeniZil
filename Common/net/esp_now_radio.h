#pragma once

#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_random.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <span>
#include "../kernel/clock.h"
#include "../kernel/component.h"
#include "../kernel/event_loop.h"
#include "protocol.h"

struct RadioConfig {                                                    // Radyo ayarları
  uint8_t  channel;                                                     // Wi-Fi kanalı (1-13), tüm ünitelerde aynı
  int8_t   txPowerDbm;                                                  // Gönderim gücü (dBm), 2-20
  bool     longRange;                                                   // Espressif Long Range modu, tüm ünitelerde aynı
  uint16_t burstPeriodMs;                                               // Bir çerçevenin kopyaları arasındaki süre (ms)
  uint16_t burstDurationMs;                                             // Bir çerçevenin tekrarlanma süresi (ms)
  uint16_t relayJitterMaxMs;                                            // Aktarmadan önceki en fazla rastgele bekleme (ms)
};

struct ReceivedFrame {                                                  // Alınan çerçeve
  static constexpr std::size_t kMaxBytes = 32;                          // En büyük çerçeve (bayt), protokol çerçevesi 20 bayt

  MacAddress                     sender{};                              // Gönderen ya da aktaran kartın MAC adresi
  int8_t                         rssi = 0;                              // Alınan sinyal gücü (dBm)
  uint8_t                        length = 0;                            // Veri uzunluğu (bayt)
  std::array<uint8_t, kMaxBytes> bytes{};                               // Veri
};

class EspNowRadio : public Component {                                  // ESP-NOW radyo: çerçeveyi süre boyunca tekrarlayarak yayınlar, alınanları kuyruğa koyar
 public:
  static constexpr std::size_t kQueueLength = 8;                        // Alma kuyruğu uzunluğu (çerçeve)
  static constexpr std::size_t kMaxBursts   = 4;                        // Aynı anda tekrarlanan en fazla çerçeve

  explicit EspNowRadio(const RadioConfig& config) : config_(config) {}

  void begin() override {
    instance_ = this;
    queue_ = xQueueCreateStatic(kQueueLength, sizeof(ReceivedFrame), queueStorage_.data(), &queueControl_);
    WiFi.mode(WIFI_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);                                      // Radyo sürekli dinler, uyanma penceresi Aşama 4'te
    if (config_.longRange) esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    esp_wifi_set_channel(config_.channel, WIFI_SECOND_CHAN_NONE);
    if (esp_now_init() != ESP_OK) {
      log_e("ESP-NOW baslatilamadi");
      return;
    }
    esp_now_register_recv_cb(&EspNowRadio::onReceive);
    addBroadcastPeer();
    if (config_.longRange) setLongRangeRate();
    esp_wifi_set_max_tx_power(static_cast<int8_t>(config_.txPowerDbm * 4));  // Sürücü 0,25 dBm birimi kullanır
    logSettings();
  }

  void update(uint64_t nowMs) override {                                // Zamanı gelen kopyaları gönderir
    for (Burst& burst : bursts_) {
      if (!burst.active || nowMs < burst.nextSendMs) continue;
      esp_now_send(kBroadcast.data(), burst.bytes.data(), burst.length);
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
    return startBurst(bytes, esp_random() % (config_.relayJitterMaxMs + 1u));
  }

  [[nodiscard]] bool receive(ReceivedFrame& frame) {                    // Kuyruktaki sıradaki çerçeveyi alır, yoksa false
    return queue_ != nullptr && xQueueReceive(queue_, &frame, 0) == pdTRUE;
  }

  static MacAddress ownMac() {                                          // Bu kartın MAC adresi
    MacAddress mac{};
    esp_read_mac(mac.data(), ESP_MAC_WIFI_STA);
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

  static constexpr MacAddress kBroadcast = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Yayın adresi

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
    log_w("Tekrar yuvalari dolu, cerceve gonderilmedi");
    return false;
  }

  static void addBroadcastPeer() {                                      // Yayın adresini ESP-NOW eşi olarak ekler
    esp_now_peer_info_t peer{};
    std::memcpy(peer.peer_addr, kBroadcast.data(), kBroadcast.size());
    peer.ifidx   = WIFI_IF_STA;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }

  static void setLongRangeRate() {                                      // Yayınları Long Range 250 kbps hızında gönderir: en uzun menzil
    esp_now_rate_config_t rate{};
    rate.phymode = WIFI_PHY_MODE_LR;
    rate.rate    = WIFI_PHY_RATE_LORA_250K;
    if (esp_now_set_peer_rate_config(kBroadcast.data(), &rate) != ESP_OK) log_e("Long Range hizi ayarlanamadi");
  }

  static void logSettings() {                                           // Sürücüden okunan gerçek ayarları loglar
    int8_t   power    = 0;
    uint8_t  protocol = 0;
    uint8_t  channel  = 0;
    wifi_second_chan_t secondary{};
    esp_wifi_get_max_tx_power(&power);
    esp_wifi_get_protocol(WIFI_IF_STA, &protocol);
    esp_wifi_get_channel(&channel, &secondary);
    log_i("Radyo: kanal %u, TX %d.%02d dBm, protokol 0x%02X%s", channel, power / 4, (power % 4) * 25, protocol,
          (protocol & WIFI_PROTOCOL_LR) ? " (Long Range)" : "");
  }

  static void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int length) {  // Wi-Fi görevinde çalışır: kopyalar, kuyruğa koyar, döngüyü uyandırır
    if (instance_ == nullptr || length <= 0 || static_cast<std::size_t>(length) > ReceivedFrame::kMaxBytes) return;
    ReceivedFrame frame;
    std::memcpy(frame.sender.data(), info->src_addr, frame.sender.size());
    frame.rssi   = static_cast<int8_t>(info->rx_ctrl->rssi);
    frame.length = static_cast<uint8_t>(length);
    std::memcpy(frame.bytes.data(), data, static_cast<std::size_t>(length));
    xQueueSend(instance_->queue_, &frame, 0);
    eventLoop.notify();
  }

  RadioConfig                     config_;                              // Radyo ayarları
  std::array<Burst, kMaxBursts>   bursts_{};                            // Tekrarlanan çerçeveler
  QueueHandle_t                   queue_ = nullptr;                     // Alma kuyruğu
  StaticQueue_t                   queueControl_{};                      // Kuyruk denetim bloğu, heap kullanılmaz
  std::array<uint8_t, kQueueLength * sizeof(ReceivedFrame)> queueStorage_{};  // Kuyruk belleği

  static inline EspNowRadio* instance_ = nullptr;                       // Geri çağrının ulaşacağı radyo
};
