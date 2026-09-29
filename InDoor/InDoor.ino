#include "indoor_unit.h"  // İç ünite nesneleri

void setup() {
  openDoorButton.onPress([] { intercom.requestDoorOpen(); });     // Kapıyı aç butonu -> dış üniteye istek
  openDoorButton.onStartupHold([] { identity.startPairing(); });  // Açılışta basılı tutuldu -> eşleştirme modu
  intercom.onRing([] { bell.ring(); });                            // Zil isteği -> zil çalar
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
