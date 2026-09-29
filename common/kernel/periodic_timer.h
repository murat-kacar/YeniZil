#pragma once

#include <cstdint>
#include "callback.h"
#include "polling_component.h"

namespace yenizil {

class PeriodicTimer : public PollingComponent {           // Sabit aralıkla olay üretir, ilk olay açılışta
 public:
  explicit PeriodicTimer(uint32_t periodMs) : PollingComponent(periodMs) {}

  void onTick(Handler<> handler) { handler_ = handler; }  // Her periyotta çağrılacak işleyiciyi bağlar

 protected:
  void poll(uint64_t) override { callIfSet(handler_); }

 private:
  Handler<> handler_ = nullptr;                           // Olay işleyicisi
};

}  // namespace yenizil
