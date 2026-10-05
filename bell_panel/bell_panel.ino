#include "bell_panel.h"  // Zil paneli nesneleri

using namespace yenizil;  // Proje isim alanı

void setup() {
  flatButtons.onPress([](NodeId flat) { panelLink.requestRing(flat); });  // N. daire butonu -> kapı ünitesine N. dairenin zil isteği
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
