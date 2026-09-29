#include "indoor_unit.h"  // İç ünite nesneleri

void setup() {
  openDoorButton.onPress([] { intercom.requestDoorOpen(); });  // Kapıyı aç butonu -> dış üniteye istek
  intercom.onRing([] { bell.ring(); });                         // Zil isteği -> zil çalar
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
