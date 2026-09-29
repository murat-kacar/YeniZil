#include "indoor.h"  // İç ünite nesneleri

using namespace yenizil;  // Proje isim alanı

void setup() {
  openDoorButton.onPress([] { intercom.requestDoorOpen(); });  // Kapıyı aç butonu -> dış üniteye istek
  intercom.onRing([] { bell.ring(); });                         // Zil isteği -> zil çalar
  intercom.onHeartbeat([] { linkMonitor.refresh(); });          // Dış üniteden "buradayım" -> bağlantı var
  linkMonitor.onConnected([] { linkLed.turnOn(); });            // Bağlantı sağlandı -> LED sürekli yanar
  linkMonitor.onLost([] { linkLed.blink(); });                  // Bağlantı yok -> LED yanıp söner
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
