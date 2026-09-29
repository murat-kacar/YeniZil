#pragma once

#include <ESP32_NOW.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include "../config/radio_settings.h"
#include "../kernel/component.h"
#include "../kernel/event_loop.h"
#include "broadcast_peer.h"
#include "protocol.h"

namespace yenizil {

struct ReceivedFrame {                                                  // Alınan çerçeve
  static constexpr std::size_t kMaxBytes = 32;                          // Radyonun kabul ettiği en büyük çerçeve (bayt), protokol çerçevesi frame::kSize

  uint8_t                        length = 0;                            // Veri uzunluğu (bayt)
  std::array<uint8_t, kMaxBytes> bytes{};                               // Veri
};

class EspNowRadio : public Component {                                  // ESP-NOW radyo: Wi-Fi'ı ve modem uykusunu ayarlar, tek kopya gönderir, alınanları kuyruğa koyar
 public:
  static constexpr std::size_t kQueueLength = 8;                        // Alma kuyruğu uzunluğu (çerçeve)

  EspNowRadio(const RadioSettings& settings, EventLoop& loop) : settings_(settings), loop_(loop), peer_(settings.longRange) {}

  void begin() override {
    queue_ = xQueueCreateStatic(kQueueLength, sizeof(ReceivedFrame), queueStorage_.data(), &queueControl_);
    WiFi.enableLongRange(settings_.longRange);                          // mode()'dan önce verilmeli
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(WIFI_PS_MIN_MODEM);                                   // Modem uykusu: radyo sadece uyanma penceresinde açık
    WiFi.setChannel(settings_.channel);
    WiFi.setTxPower(static_cast<wifi_power_t>(settings_.txPowerDbm * 4));  // Sürücü 0,25 dBm birimi kullanır
    if (!ESP_NOW.begin()) return;
    ESP_NOW.onNewPeer(&EspNowRadio::onReceive, this);                   // Göndericiler eş olarak eklenmez, tüm çerçeveler buraya gelir
    if (!peer_.begin()) return;
    esp_now_set_wake_window(settings_.wakeWindowMs);                    // Her aralıkta bu kadar dinler (Arduino karşılığı yok)
    esp_wifi_connectionless_module_set_wake_interval(settings_.wakeIntervalMs);  // Uyanma aralığı (Arduino karşılığı yok)
  }

  void update(uint64_t) override {}

  bool send(std::span<const uint8_t> bytes) { return peer_.send(bytes); }  // Tek kopya yayınlar

  [[nodiscard]] bool receive(ReceivedFrame& frame) {                    // Kuyruktaki sıradaki çerçeveyi alır, yoksa false
    return queue_ != nullptr && xQueueReceive(queue_, &frame, 0) == pdTRUE;
  }

  static MacAddress ownMac() {                                          // Bu kartın MAC adresi, begin() sonrası geçerli
    MacAddress mac{};
    WiFi.macAddress(mac.data());
    return mac;
  }

 private:
  static void onReceive(const esp_now_recv_info_t*, const uint8_t* data, int length, void* arg) {  // Wi-Fi görevinde çalışır: kopyalar, kuyruğa koyar, döngüyü uyandırır
    if (length <= 0 || static_cast<std::size_t>(length) > ReceivedFrame::kMaxBytes) return;
    ReceivedFrame frame;
    frame.length = static_cast<uint8_t>(length);
    std::copy_n(data, length, frame.bytes.begin());
    EspNowRadio& radio = *static_cast<EspNowRadio*>(arg);
    xQueueSend(radio.queue_, &frame, 0);
    radio.loop_.notify();
  }

  RadioSettings  settings_;                                             // Radyo ayarları
  EventLoop&     loop_;                                                 // Çerçeve gelince uyandırılacak döngü
  BroadcastPeer  peer_;                                                 // Yayın eşi
  QueueHandle_t  queue_ = nullptr;                                      // Alma kuyruğu
  StaticQueue_t  queueControl_{};                                       // Kuyruk denetim bloğu, heap kullanılmaz
  std::array<uint8_t, kQueueLength * sizeof(ReceivedFrame)> queueStorage_{};  // Kuyruk belleği
};

}  // namespace yenizil
