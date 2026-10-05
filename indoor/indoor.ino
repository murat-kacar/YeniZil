#include "indoor.h"  // İç ünite nesneleri

using namespace yenizil;  // Proje isim alanı

void setup() {
  openDoorButton.onPress([] { intercom.requestDoorOpen(); });  // Kapıyı aç butonu -> kapı ünitesine istek
  intercom.onRing([] { bell.ring(); });                         // Zil isteği -> zil çalar
  intercom.onHeartbeat([] { linkLed.activate(); });             // Kapı ünitesinden "buradayım" -> bağlantı LED'i kısa yanar
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
