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

  void begin() override {                                 // Kimliği MAC tablosundan bulur; rolle uyuşmazsa ya da güvenlik başlamazsa ağ kapalı kalır
    const std::optional<NodeId> self = findNodeId(site::kNodeMacs, EspNowRadio::ownMac());
    if (!self || !matchesRole(*self) || !channel_.begin(*self)) return;
    self_  = *self;
    ready_ = true;
  }

  void update(uint64_t) override {                        // Alınan çerçeveleri işler
    ReceivedFrame received;
    while (radio_.receive(received))
      if (ready_) handle(received);
  }

  void send(NodeId destination, MessageType type) {       // Hedefe mesaj gönderir
    if (!ready_ || destination >= kNodeCount || destination == self_) return;
    if (const std::optional<frame::Bytes> bytes = channel_.seal(destination, type)) radio_.broadcast(*bytes);
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
      deliver(*message);
      return;
    }
    radio_.relay(bytes);                                  // Çerçeve değiştirilmeden aktarılır
  }

  void deliver(const Message& message) {                  // Kendine gelen mesajı işleyicisine verir
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
