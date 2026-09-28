#include "bench.h"  // Tezgâh nesneleri

void setup() {
  Serial.begin(115200);
  flatButtons.onPress(bench::reportFlatPress);            // Daire butonu -> log + kapı çıkışı
  bench::singleButton.onPress(bench::reportSinglePress);  // Tek buton -> log + zil çıkışı
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
