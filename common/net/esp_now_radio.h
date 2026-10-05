#pragma once

#include <ESP32_NOW.h>
#include <WiFi.h>
#include <esp_now.h>
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
#include "frame.h"
#include "frame_receiver.h"
#include "protocol.h"

namespace yenizil {

class EspNowRadio : public Component {                                  // ESP-NOW radyo: Wi-Fi'ı ayarlar, sürekli dinler, tek kopya gönderir, alınanları bağlı ağlara dağıtır
 public:
  static constexpr std::size_t kQueueLength       = 8;                  // Alma kuyruğu uzunluğu (çerçeve)
  static constexpr int         kTxPowerStepsPerDbm = 4;                  // Sürücünün TX gücü birimi: 0,25 dBm

  EspNowRadio(const RadioSettings& settings, EventLoop& loop) : settings_(settings), loop_(loop), peer_(settings.longRange) {}

  void begin() override {
    queue_ = xQueueCreateStatic(kQueueLength, sizeof(frame::Bytes), queueStorage_.data(), &queueControl_);
    WiFi.enableLongRange(settings_.longRange);                          // mode()'dan önce verilmeli
    ready_ = queue_ != nullptr                                          // Adımlardan biri başarısızsa radyo kapalı kalır
          && WiFi.mode(WIFI_STA)
          && WiFi.setSleep(false)                                       // Modem uykusu kapalı: radyo sürekli dinler, mesaj ilk kopyada yakalanır
          && WiFi.setChannel(settings_.channel) == ESP_OK
          && WiFi.setTxPower(static_cast<wifi_power_t>(settings_.txPowerDbm * kTxPowerStepsPerDbm))
          && ESP_NOW.begin()
          && peer_.begin();
    if (ready_) ESP_NOW.onNewPeer(&EspNowRadio::onReceive, this);       // Göndericiler eş olarak eklenmez, tüm çerçeveler buraya gelir
  }

  void update(uint64_t) override {                                      // Kuyruktaki çerçeveleri bağlı ağlara verir
    frame::Bytes bytes{};
    while (queue_ != nullptr && xQueueReceive(queue_, &bytes, 0) == pdTRUE)
      for (FrameReceiver* receiver = receivers_; receiver != nullptr; receiver = receiver->nextReceiver_) receiver->receive(bytes);
  }

  void attach(FrameReceiver& receiver) {                                // Ağı alıcı listesine ekler, ağın constructor'ı çağırır
    receiver.nextReceiver_ = receivers_;
    receivers_             = &receiver;
  }

  [[nodiscard]] bool isReady() const { return ready_; }                // Kurulum tamamlandı mı

  [[nodiscard]] bool send(std::span<const uint8_t> bytes) { return ready_ && peer_.send(bytes); }  // Tek kopya yayınlar, gönderilemezse false

  static MacAddress ownMac() {                                          // Bu kartın MAC adresi, begin() sonrası geçerli
    MacAddress mac{};
    WiFi.macAddress(mac.data());
    return mac;
  }

 private:
  static void onReceive(const esp_now_recv_info_t*, const uint8_t* data, int length, void* arg) {  // Wi-Fi görevinde çalışır: kopyalar, kuyruğa koyar, döngüyü uyandırır
    if (length != static_cast<int>(frame::kSize)) return;              // Protokol çerçevesi olmayan veri kuyruğa girmez
    frame::Bytes bytes{};
    std::copy_n(data, frame::kSize, bytes.begin());
    EspNowRadio& radio = *static_cast<EspNowRadio*>(arg);
    if (xQueueSend(radio.queue_, &bytes, 0) != pdTRUE) return;          // Kuyruk doluysa çerçeve düşer: gönderen her çerçeveyi 3 kez yollar
    radio.loop_.notify();
  }

  RadioSettings  settings_;                                             // Radyo ayarları
  EventLoop&     loop_;                                                 // Çerçeve gelince uyandırılacak döngü
  BroadcastPeer  peer_;                                                 // Yayın eşi
  bool           ready_     = false;                                    // Kurulum tamamlandı mı
  FrameReceiver* receivers_ = nullptr;                                  // Bağlı ağların listesi
  QueueHandle_t  queue_     = nullptr;                                  // Alma kuyruğu
  StaticQueue_t  queueControl_{};                                       // Kuyruk denetim bloğu, heap kullanılmaz
  std::array<uint8_t, kQueueLength * sizeof(frame::Bytes)> queueStorage_{};  // Kuyruk belleği
};

}  // namespace yenizil
