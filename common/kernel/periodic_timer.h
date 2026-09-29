#pragma once

#include "polling_component.h"

class PeriodicTimer : public PollingComponent {           // Sabit aralıkla olay üretir, ilk olay açılışta
 public:
  using Handler = void (*)();                             // Olay işleyicisi

  explicit PeriodicTimer(uint32_t periodMs) : PollingComponent(periodMs) {}

  void onTick(Handler handler) { handler_ = handler; }    // Her periyotta çağrılacak işleyiciyi bağlar

 protected:
  void poll(uint64_t) override {
    if (handler_ != nullptr) handler_();
  }

 private:
  Handler handler_ = nullptr;                             // Olay işleyicisi
};
