#pragma once

#include <cstdint>
#include "../kernel/callback.h"
#include "../kernel/clock.h"
#include "../kernel/component.h"

namespace yenizil {

class LinkMonitor : public Component {                    // Bağlantı denetimi: süre içinde sinyal geldiyse bağlı, gelmezse koptu
 public:
  explicit LinkMonitor(uint32_t timeoutMs) : timeoutMs_(timeoutMs) {}

  void onConnected(Handler<> handler) { connectedHandler_ = handler; }  // Bağlantı sağlanınca çağrılır
  void onLost(Handler<> handler) { lostHandler_ = handler; }            // Bağlantı kopunca ve açılışta çağrılır

  void begin() override { callIfSet(lostHandler_); }      // Açılışta bağlantı yok

  void refresh() {                                        // Sinyal geldi: süreyi yeniler, bağlı değilse bağlanır
    expiresAtMs_ = monotonicMs() + timeoutMs_;
    if (connected_) return;
    connected_ = true;
    callIfSet(connectedHandler_);
  }

  void update(uint64_t nowMs) override {                  // Süre dolduysa bağlantı koptu
    if (!connected_ || nowMs < expiresAtMs_) return;
    connected_ = false;
    callIfSet(lostHandler_);
  }

  uint64_t nextDeadlineMs() const override { return connected_ ? expiresAtMs_ : kNoDeadlineMs; }

 private:
  uint32_t  timeoutMs_;                                   // Sinyalsiz geçebilecek en uzun süre (ms)
  bool      connected_        = false;                    // Bağlı mı
  uint64_t  expiresAtMs_      = 0;                        // Bağlantının düşeceği an
  Handler<> connectedHandler_ = nullptr;                  // Bağlandı işleyicisi
  Handler<> lostHandler_      = nullptr;                  // Koptu işleyicisi
};

}  // namespace yenizil
