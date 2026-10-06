#include "door_unit.h"  // Kapı ünitesi nesneleri

using namespace yenizil;  // Proje isim alanı

void setup() {
  panelLink.onRingRequest([](NodeId flat) { intercom.ringFlat(flat); });  // Zil panelinden N. daire isteği -> N. dairenin zili
  intercom.onDoorOpenRequest([] { doorOpener.open(); });                  // Kapı açma isteği -> kapı açılır
  heartbeatTimer.onTick([] { intercom.broadcastHeartbeat(); });           // Periyot doldu -> "buradayım" yayını
  pairingButton.onPress([] { pairing.open(); });                          // Eşleştirme butonu 10-15 sn basılı tutulup bırakıldı -> eşleştirme modu açılır
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
