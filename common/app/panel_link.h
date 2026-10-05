#pragma once

#include <cstdint>
#include <optional>
#include "../kernel/callback.h"
#include "../kernel/component.h"
#include "../net/burst_sender.h"
#include "../net/esp_now_radio.h"
#include "../net/frame.h"
#include "../net/frame_receiver.h"
#include "../net/nodes.h"
#include "../net/protocol.h"
#include "../security/ccm_cipher.h"
#include "../security/network_credentials.h"
#include "../security/secure_channel.h"
#include "radio_stack.h"

namespace yenizil {

class PanelLink : public Component, public FrameReceiver {  // Zil paneli bağlantısı: panel sadece kapı ünitesine zil isteği yollar, aktarma yok. Ayrı ağ ve şifre: panel ele geçse kapı açılamaz
 public:
  PanelLink(RadioStack& stack, const NetworkCredentials& link)
      : radio_(stack.radio()), bursts_(stack.bursts()), cipher_(link.key), channel_(cipher_, stack.store(), stack.txCounter(), link.id) {
    radio_.attach(*this);
  }

  void begin() override { ready_ = radio_.isReady() && channel_.begin(EspNowRadio::ownMac()); }  // Radyo ya da güvenlik başlamazsa bağlantı kapalı kalır
  void update(uint64_t) override {}

  void requestRing(NodeId flat) {                         // Zil paneli: kapı ünitesinden dairenin zilini çaldırmasını ister
    if (!ready_ || !isFlatId(flat)) return;
    if (const std::optional<frame::Bytes> bytes = channel_.seal(kBellPanelId, flat, MessageType::kRingBell))
      static_cast<void>(bursts_.send(*bytes));             // Yuvalar doluysa istek düşer: zil beklemesi aynı anda 4'ten fazla isteği önler
  }

  void onRingRequest(Handler<NodeId> handler) { handler_ = handler; }  // Kapı ünitesi: panelden zil isteği gelince, daire numarasıyla

 private:
  void receive(const frame::Bytes& bytes) override {      // Tek çerçeve: doğrula, sadece panelin zil isteğini kabul et
    if (!ready_) return;
    const std::optional<Message> message = channel_.open(bytes);
    if (!message || message->source != kBellPanelId || message->type != MessageType::kRingBell || !isFlatId(message->destination)) return;  // Panelden başka istek yok
    if (channel_.commit(*message)) callIfSet(handler_, message->destination);  // Eylemden önce kalıcı kayıt: kaydedilemezse eylem yok
  }

  EspNowRadio&     radio_;                                // Radyo: kurulum durumu, kartın MAC'i
  BurstSender&     bursts_;                               // Tekrarlı gönderim
  CcmCipher        cipher_;                               // Bağlantının AES-CCM'i
  SecureChannel    channel_;                              // Çerçeve üretimi ve doğrulama
  bool             ready_   = false;                      // Güvenlik başladı, bağlantı açık
  Handler<NodeId>  handler_ = nullptr;                    // Zil isteği işleyicisi
};

}  // namespace yenizil
