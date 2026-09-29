#pragma once

#include <cstdint>
#include "component.h"

namespace yenizil {

class PollingComponent : public Component {                // Sabit aralıklarla örnekleme yapan bileşen (Template Method)
 public:
  explicit PollingComponent(uint32_t periodMs) : periodMs_(periodMs) {}

  void update(uint64_t nowMs) final {
    if (nowMs < nextPollMs_) return;
    nextPollMs_ = nowMs + periodMs_;
    poll(nowMs);
  }

  uint64_t nextDeadlineMs() const final { return nextPollMs_; }

 protected:
  virtual void poll(uint64_t nowMs) = 0;                   // Her periyotta bir kez çağrılır

 private:
  uint32_t periodMs_;                                      // Örnekleme periyodu (ms)
  uint64_t nextPollMs_ = 0;                                // Bir sonraki örnekleme zamanı
};

}  // namespace yenizil
