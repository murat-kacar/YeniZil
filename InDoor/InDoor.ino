#include "indoor_unit.h"  // İç ünite nesneleri

void setup() {
  openDoorButton.onPress([] { intercom.requestDoorOpen(); });     // Kapıyı aç butonu -> dış üniteye istek
  openDoorButton.onStartupHold([] { identity.startPairing(); });  // Açılışta basılı tutuldu -> eşleştirme modu
  intercom.onRing([] { bell.ring(); });                            // Zil isteği -> zil çalar
  intercom.onHeartbeat([] { linkMonitor.refresh(); });             // Dış üniteden "buradayım" -> bağlantı var
  linkMonitor.onConnected([] { linkLed.turnOn(); });               // Bağlantı sağlandı -> LED sürekli yanar
  linkMonitor.onLost([] { linkLed.blink(); });                     // Bağlantı yok -> LED yanıp söner
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
