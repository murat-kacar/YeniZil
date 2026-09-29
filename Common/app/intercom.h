#pragma once

#include "../net/flood_router.h"
#include "../net/protocol.h"

class Intercom {                                          // Diyafon: protokolü gizler, alan dilinde işlemler sunar (Facade)
 public:
  using Handler = void (*)();                             // Olay işleyicisi

  explicit Intercom(FloodRouter& router) : router_(router) {}

  void ringFlat(NodeId flat) { router_.send(flat, MessageType::kRingBell); }                       // Dairenin zilini çaldırır
  void requestDoorOpen() { router_.send(kOutdoorUnitId, MessageType::kOpenDoor); }                 // Dış üniteden kapıyı açmasını ister
  void onRing(Handler handler) { router_.on(MessageType::kRingBell, handler); }                    // Zil isteği gelince
  void onDoorOpenRequest(Handler handler) { router_.on(MessageType::kOpenDoor, handler); }         // Kapı açma isteği gelince

 private:
  FloodRouter& router_;                                   // Ağ
};
