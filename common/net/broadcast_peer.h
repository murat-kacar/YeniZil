#pragma once

#include <ESP32_NOW.h>
#include <cstdint>
#include <span>
#include "protocol.h"

namespace yenizil {

class BroadcastPeer : public ESP_NOW_Peer {               // ESP-NOW yayın eşi (Arduino ESP_NOW): herkese gönderir
 public:
  explicit BroadcastPeer(bool longRange) : BroadcastPeer(rateFor(longRange)) {}

  [[nodiscard]] bool begin() { return add(); }            // Eşi ekler, ESP_NOW.begin() sonrası çağrılır

  bool send(std::span<const uint8_t> bytes) {             // Tek kopya yayınlar
    return ESP_NOW_Peer::send(bytes.data(), static_cast<int>(bytes.size())) == bytes.size();
  }

 private:
  static constexpr MacAddress kBroadcastMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Yayın adresi

  explicit BroadcastPeer(esp_now_rate_config_t rate) : ESP_NOW_Peer(kBroadcastMac.data(), 0, WIFI_IF_STA, nullptr, &rate) {}  // Kanal 0: Wi-Fi'ın kanalı

  static esp_now_rate_config_t rateFor(bool longRange) {  // Long Range açıksa 250 kbps: en uzun menzil
    esp_now_rate_config_t rate = DEFAULT_ESPNOW_RATE_CONFIG;
    if (longRange) {
      rate.phymode = WIFI_PHY_MODE_LR;
      rate.rate    = WIFI_PHY_RATE_LORA_250K;
    }
    return rate;
  }
};

}  // namespace yenizil
