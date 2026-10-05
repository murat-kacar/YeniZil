#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include "../kernel/callback.h"
#include "../kernel/component.h"
#include "../kernel/enum_value.h"
#include "../security/secure_channel.h"
#include "burst_sender.h"
#include "esp_now_radio.h"
#include "frame.h"
#include "frame_receiver.h"
#include "nodes.h"
#include "protocol.h"

namespace yenizil {

class FloodRouter : public Component, public FrameReceiver {  // Flooding: kendine geleni teslim eder, gerisini değiştirmeden aktarır
 public:
  FloodRouter(EspNowRadio& radio, BurstSender& bursts, SecureChannel& channel, NodeId self)
      : radio_(radio), bursts_(bursts), channel_(channel), self_(self) {
    radio_.attach(*this);
  }

  void begin() override { ready_ = radio_.isReady() && channel_.begin(EspNowRadio::ownMac()); }  // Radyo ya da güvenlik başlamazsa ağ kapalı kalır
  void update(uint64_t) override {}

  void send(NodeId destination, MessageType type) {       // Hedefe mesaj gönderir
    if (!ready_ || !isDestination(destination) || destination == self_ || !matchesAddressing(type, destination)) return;
    if (const std::optional<frame::Bytes> bytes = channel_.seal(self_, destination, type))
      static_cast<void>(bursts_.send(*bytes));             // Yuvalar doluysa mesaj düşer: aynı anda 4'ten fazla gönderim olağan kullanımda olmaz
  }

  void on(MessageType type, Handler<> handler) { handlers_[toUnderlying(type)] = handler; }  // Mesaj tipine işleyici bağlar

 private:
  void receive(const frame::Bytes& bytes) override {      // Tek çerçeve: doğrula, sonra teslim et ya da aktar
    if (!ready_) return;
    const std::optional<Message> message = channel_.open(bytes);
    if (!message || message->source == kBellPanelId || !matchesAddressing(message->type, message->destination)) return;  // Zil paneli bina ağında değil
    if (message->destination == kAllUnitsId) {            // Herkese: aktar ve teslim et, eylem olmadığı için kalıcı kayıt yok (flash aşınmaz)
      static_cast<void>(bursts_.relay(bytes));            // Yuva doluysa aktarılmaz: diğer üniteler de aktarır (flooding yedekliliği)
      deliver(*message);
      return;
    }
    if (message->destination != self_) {
      static_cast<void>(bursts_.relay(bytes));            // Değiştirilmeden aktarılır, yuva doluysa diğer üniteler aktarır
      return;
    }
    if (channel_.commit(*message)) deliver(*message);     // Eylemden önce kalıcı kayıt: kaydedilemezse eylem yok
  }

  void deliver(const Message& message) { callIfSet(handlers_[toUnderlying(message.type)]); }  // Mesajı işleyicisine verir

  EspNowRadio&                               radio_;      // Radyo: kurulum durumu, kartın MAC'i
  BurstSender&                               bursts_;     // Tekrarlı gönderim ve aktarma
  SecureChannel&                             channel_;    // Çerçeve üretimi ve doğrulama
  NodeId                                     self_;       // Bu ünitenin kimliği: kapı ünitesi 0, iç ünite kFlatId
  bool                                       ready_ = false;  // Güvenlik başladı, ağ açık
  std::array<Handler<>, kMessageTypeCount>   handlers_{}; // Tip başına işleyici
};

}  // namespace yenizil
