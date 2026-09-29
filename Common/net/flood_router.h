#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include "../kernel/component.h"
#include "../security/secure_channel.h"
#include "esp_now_radio.h"
#include "frame.h"
#include "node_identity.h"
#include "nodes.h"
#include "protocol.h"

static_assert(frame::kSize <= ReceivedFrame::kMaxBytes, "Çerçeve radyonun alma tamponuna sığmalı");

class FloodRouter : public Component {                    // Flooding: kendine geleni teslim eder, gerisini değiştirmeden aktarır
 public:
  using Handler = void (*)();                             // Mesaj işleyicisi

  FloodRouter(EspNowRadio& radio, SecureChannel& channel, NodeIdentity& identity) : radio_(radio), channel_(channel), identity_(identity) {}

  void begin() override { ready_ = channel_.begin(EspNowRadio::ownMac()); }  // Güvenlik başlamazsa ağ kapalı kalır

  void update(uint64_t) override {                        // Alınan çerçeveleri işler
    ReceivedFrame received;
    while (radio_.receive(received))
      if (ready_) handle(received);
  }

  void send(NodeId destination, MessageType type) {       // Hedefe mesaj gönderir, eşleşmemiş ünite gönderemez
    const std::optional<NodeId> self = identity_.id();
    if (!ready_ || !self || !isDestination(destination) || destination == *self) return;
    if (const std::optional<frame::Bytes> bytes = channel_.seal(*self, destination, type)) radio_.broadcast(*bytes);
  }

  void on(MessageType type, Handler handler) { handlers_[static_cast<std::size_t>(type)] = handler; }  // Mesaj tipine işleyici bağlar

 private:
  void handle(const ReceivedFrame& received) {            // Tek çerçeve: doğrula, sonra teslim et ya da aktar
    if (received.length != frame::kSize) return;
    frame::Bytes bytes{};
    std::copy_n(received.bytes.begin(), frame::kSize, bytes.begin());
    const std::optional<Message> message = channel_.open(bytes);
    if (!message) return;
    if (identity_.isPairing() && message->type == MessageType::kRingBell && isFlatId(message->destination))
      identity_.adopt(message->destination);              // Eşleştirme (learn mode): gelen ilk zil isteğinin dairesi bu ünitenin kimliği olur
    if (message->destination == kAllUnitsId) {            // Herkese: aktar ve teslim et. Sadece heartbeat, eylem olmadığı için kalıcı kayıt yok (flash aşınmaz)
      radio_.relay(bytes);
      if (message->type == MessageType::kHeartbeat) deliver(*message);
      return;
    }
    if (message->destination != identity_.id()) {
      radio_.relay(bytes);                                // Başkasının çerçevesi değiştirilmeden aktarılır
      return;
    }
    if (channel_.commit(*message)) deliver(*message);     // Eylemden önce kalıcı kayıt: kaydedilemezse eylem yok
  }

  void deliver(const Message& message) {                  // Kendine gelen mesajı işleyicisine verir
    const Handler handler = handlers_[static_cast<std::size_t>(message.type)];
    if (handler != nullptr) handler();
  }

  EspNowRadio&                              radio_;       // Radyo
  SecureChannel&                            channel_;     // Çerçeve üretimi ve doğrulama
  NodeIdentity&                             identity_;    // Bu ünitenin kimliği
  bool                                      ready_ = false;  // Güvenlik başladı, ağ açık
  std::array<Handler, kMessageTypeCount>    handlers_{};  // Tip başına işleyici
};
