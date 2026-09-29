#include "outdoor_unit.h"  // Dış ünite nesneleri

void setup() {
  flatButtons.onPress([](NodeId flat) { intercom.ringFlat(flat); });  // N. daire butonu -> N. dairenin zili
  intercom.onDoorOpenRequest([] { doorOpener.open(); });              // Kapı açma isteği -> kapı açılır
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
