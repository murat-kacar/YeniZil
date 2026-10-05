#pragma once

#include "../config/radio_settings.h"
#include "../kernel/event_loop.h"
#include "../net/burst_sender.h"
#include "../net/esp_now_radio.h"
#include "../security/counter_store.h"
#include "../security/tx_counter.h"

namespace yenizil {

class RadioStack {                                        // Kartın ortak radyo katmanı: radyo, tekrarlı gönderim, kalıcı sayaçlar. Karttaki ağlar (Intercom, PanelLink) bunu paylaşır
 public:
  RadioStack(const RadioSettings& radio, const BurstSettings& burst, EventLoop& loop)
      : radio_(radio, loop), bursts_(radio_, burst), txCounter_(store_) {}

  EspNowRadio&  radio() { return radio_; }                // Radyo
  BurstSender&  bursts() { return bursts_; }              // Tekrarlı gönderim
  CounterStore& store() { return store_; }                // Sayaç kalıcı kaydı
  TxCounter&    txCounter() { return txCounter_; }        // Kartın gönderme sayacı

 private:
  EspNowRadio  radio_;                                    // Radyo, bileşen sırası: ağlardan önce başlar
  BurstSender  bursts_;                                   // Tekrarlı gönderim
  CounterStore store_;                                    // Sayaç kalıcı kaydı
  TxCounter    txCounter_;                                // Gönderme sayacı: tek sayaç, ağlar arasında nonce tekrarlanmaz
};

}  // namespace yenizil
