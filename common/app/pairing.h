#pragma once

#include <cstdint>
#include <optional>
#include "../kernel/clock.h"
#include "../kernel/component.h"

namespace yenizil {

class Pairing : public Component {                        // Eşleştirme modu: açılınca süre boyunca açık kalır, süre dolunca kendiliğinden kapanır (Zigbee permit join)
 public:
  explicit Pairing(uint32_t windowMs) : windowMs_(windowMs) {}

  void open() { closeAtMs_ = monotonicMs() + windowMs_; } // Eşleştirme modunu açar, açıksa süreyi baştan başlatır
  [[nodiscard]] bool isOpen() const { return closeAtMs_.has_value(); }  // Eşleştirme modu açık mı

  void update(uint64_t nowMs) override {
    if (closeAtMs_ && nowMs >= *closeAtMs_) closeAtMs_.reset();
  }

  uint64_t nextDeadlineMs() const override { return closeAtMs_.value_or(kNoDeadlineMs); }

 private:
  uint32_t                windowMs_;                      // Eşleştirme modunun açık kalma süresi (ms)
  std::optional<uint64_t> closeAtMs_;                     // Kapanacağı an, kapalıysa boş
};

}  // namespace yenizil
