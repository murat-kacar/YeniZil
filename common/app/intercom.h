#pragma once

#include "../kernel/callback.h"
#include "../net/flood_router.h"
#include "../net/protocol.h"
#include "../security/ccm_cipher.h"
#include "../security/network_credentials.h"
#include "../security/secure_channel.h"
#include "radio_stack.h"

namespace yenizil {

class Intercom {                                          // Diyafon: bina ağını (kapı ünitesi + iç üniteler) kurar ve gizler, alan dilinde işlemler sunar (Facade)
 public:
  Intercom(RadioStack& stack, const NetworkCredentials& network, NodeId self)
      : cipher_(network.key),
        channel_(cipher_, stack.store(), stack.txCounter(), network.id),
        router_(stack.radio(), stack.bursts(), channel_, self) {}

  void ringFlat(NodeId flat) { router_.send(flat, MessageType::kRingBell); }                       // Dairenin zilini çaldırır
  void requestDoorOpen() { router_.send(kDoorUnitId, MessageType::kOpenDoor); }                    // Kapı ünitesinden kapıyı açmasını ister
  void broadcastHeartbeat() { router_.send(kAllUnitsId, MessageType::kHeartbeat); }                // Kapı ünitesi: ağda olduğunu duyurur
  void onRing(Handler<> handler) { router_.on(MessageType::kRingBell, handler); }                  // Zil isteği gelince
  void onDoorOpenRequest(Handler<> handler) { router_.on(MessageType::kOpenDoor, handler); }       // Kapı açma isteği gelince
  void onHeartbeat(Handler<> handler) { router_.on(MessageType::kHeartbeat, handler); }            // Kapı ünitesinden "buradayım" gelince

 private:
  CcmCipher     cipher_;                                  // Bina ağının AES-CCM'i
  SecureChannel channel_;                                 // Çerçeve üretimi ve doğrulama
  FloodRouter   router_;                                  // Flooding, radyodan sonra başlar: MAC'i radyo açılınca okur
};

}  // namespace yenizil
