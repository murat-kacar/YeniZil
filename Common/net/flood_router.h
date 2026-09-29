#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include "../config/site_config.h"
#include "../kernel/component.h"
#include "../security/secure_channel.h"
#include "esp_now_radio.h"
#include "frame.h"
#include "node_identity.h"
#include "nodes.h"
#include "protocol.h"

enum class NodeRole : uint8_t {                           // Firmware rolü
  kOutdoor,                                               // Dış ünite: kimlik 0 olmalı
  kIndoor,                                                // İç ünite: kimlik 1 ve üstü olmalı
};

class FloodRouter : public Component {                    // Flooding: kendine geleni teslim eder, gerisini değiştirmeden aktarır
 public:
  using Handler = void (*)();                             // Mesaj işleyicisi

  FloodRouter(EspNowRadio& radio, SecureChannel& channel, NodeRole role) : radio_(radio), channel_(channel), role_(role) {}

  void begin() override {                                 // Kimliği MAC tablosundan bulur, rolle uyuşmazsa ağı kapalı tutar
    const MacAddress mac = EspNowRadio::ownMac();
    const std::optional<NodeId> self = findNodeId(site::kNodeMacs, mac);
    if (!self || !matchesRole(*self)) {
      log_e("Bu kart (%02X:%02X:%02X:%02X:%02X:%02X) %s. Ag kapali; site_config.h kNodeMacs tablosunu duzeltin.", mac[0], mac[1], mac[2], mac[3],
            mac[4], mac[5], self ? "tabloda baska bir rolde" : "MAC tablosunda yok");
      return;
    }
    self_ = *self;
    channel_.begin(self_);
    ready_ = true;
    log_i("Kimlik %u (%s)", self_, self_ == kOutdoorUnitId ? "dis unite" : "daire");
  }

  void update(uint64_t) override {                        // Alınan çerçeveleri işler
    ReceivedFrame received;
    while (radio_.receive(received))
      if (ready_) handle(received);
  }

  void send(NodeId destination, MessageType type) {       // Hedefe mesaj gönderir
    if (!ready_ || destination >= kNodeCount || destination == self_) {
      log_w("Gonderilmedi: hedef %u gecersiz ya da ag kapali", destination);
      return;
    }
    const frame::Bytes bytes = channel_.seal(destination, type);
    radio_.broadcast(bytes);
    log_i("Gonderildi: tip %u -> %u", static_cast<unsigned>(type), destination);
  }

  void on(MessageType type, Handler handler) { handlers_[static_cast<std::size_t>(type)] = handler; }  // Mesaj tipine işleyici bağlar

 private:
  bool matchesRole(NodeId self) const { return (role_ == NodeRole::kOutdoor) == (self == kOutdoorUnitId); }  // Kimlik firmware rolüyle uyuşuyor mu

  void handle(const ReceivedFrame& received) {            // Tek çerçeve: doğrula, sonra teslim et ya da aktar
    if (received.length != frame::kSize) return;
    frame::Bytes bytes{};
    std::copy_n(received.bytes.begin(), frame::kSize, bytes.begin());
    const std::optional<Message> message = channel_.open(bytes);
    if (!message) return;
    if (message->destination == self_) {
      deliver(*message, received.rssi);
      return;
    }
    radio_.relay(bytes);                                  // Çerçeve değiştirilmeden aktarılır
    log_i("Aktarildi: %u -> %u, sayac %lu, RSSI %d", message->source, message->destination, static_cast<unsigned long>(message->counter), received.rssi);
  }

  void deliver(const Message& message, int8_t rssi) {     // Kendine gelen mesajı işleyicisine verir
    log_i("Alindi: tip %u, kaynak %u, sayac %lu, RSSI %d", static_cast<unsigned>(message.type), message.source,
          static_cast<unsigned long>(message.counter), rssi);
    const Handler handler = handlers_[static_cast<std::size_t>(message.type)];
    if (handler != nullptr) handler();
  }

  EspNowRadio&                              radio_;       // Radyo
  SecureChannel&                            channel_;     // Çerçeve üretimi ve doğrulama
  NodeRole                                  role_;        // Firmware rolü
  NodeId                                    self_  = 0;   // Bu ünitenin kimliği
  bool                                      ready_ = false;  // Kimlik bulundu, ağ açık
  std::array<Handler, kMessageTypeCount>    handlers_{};  // Tip başına işleyici
};
