#pragma once

#include "../net/flood_router.h"
#include "../net/protocol.h"

class Intercom {                                          // Diyafon: protokolü gizler, alan dilinde işlemler sunar (Facade)
 public:
  using Handler = FloodRouter::Handler;                   // Olay işleyicisi

  explicit Intercom(FloodRouter& router) : router_(router) {}

  void ringFlat(NodeId flat) { router_.send(flat, MessageType::kRingBell); }                       // Dairenin zilini çaldırır
  void requestDoorOpen() { router_.send(kOutdoorUnitId, MessageType::kOpenDoor); }                 // Dış üniteden kapıyı açmasını ister
  void onRing(Handler handler) { router_.on(MessageType::kRingBell, handler); }                    // Zil isteği gelince
  void onDoorOpenRequest(Handler handler) { router_.on(MessageType::kOpenDoor, handler); }         // Kapı açma isteği gelince
  void broadcastHeartbeat() { router_.send(kAllUnitsId, MessageType::kHeartbeat); }                // Dış ünite: ağda olduğunu duyurur
  void onHeartbeat(Handler handler) { router_.on(MessageType::kHeartbeat, handler); }              // Dış üniteden "buradayım" gelince

 private:
  FloodRouter& router_;                                   // Ağ
};
