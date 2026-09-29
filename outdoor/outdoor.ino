#include "outdoor.h"  // Dış ünite nesneleri

using namespace yenizil;  // Proje isim alanı

void setup() {
  flatButtons.onPress([](NodeId flat) { intercom.ringFlat(flat); });  // N. daire butonu -> N. dairenin zili
  intercom.onDoorOpenRequest([] { doorOpener.open(); });              // Kapı açma isteği -> kapı açılır
  heartbeatTimer.onTick([] { intercom.broadcastHeartbeat(); });       // Periyot doldu -> "buradayım" yayını
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
