#include "indoor.h"  // İç ünite nesneleri

using namespace yenizil;  // Proje isim alanı

void setup() {
  openDoorButton.onPress([] { intercom.requestDoorOpen(); });  // Kapıyı aç butonu -> dış üniteye istek
  intercom.onRing([] { bell.ring(); });                         // Zil isteği -> zil çalar
  intercom.onHeartbeat([] { linkLed.activate(); });             // Dış üniteden "buradayım" -> bağlantı LED'i kısa yanar
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
