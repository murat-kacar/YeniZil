#pragma once

#include <Arduino.h>
#include <driver/gpio.h>
#include "../kernel/clock.h"
#include "../kernel/component.h"

class PulseOutput : public Component {                    // Belirli süre aktif kalan çıkış
 public:
  PulseOutput(uint8_t pin, uint8_t activeLevel, uint32_t durationMs)
      : pin_(pin), activeLevel_(activeLevel), durationMs_(durationMs) {}

  void begin() override {
    gpio_set_level(static_cast<gpio_num_t>(pin_), inactiveLevel());  // Çıkış açılmadan önce pasif seviye yazılır, açılışta titreme olmaz
    pinMode(pin_, OUTPUT);
    write(false);
  }

  void activate() {                                       // Çıkışı süre boyunca aktif yapar, aktifken gelen çağrı yok sayılır
    if (active_) return;
    active_ = true;
    offAtMs_ = monotonicMs() + durationMs_;
    write(true);
  }

  void update(uint64_t nowMs) override {
    if (!active_ || nowMs < offAtMs_) return;
    active_ = false;
    write(false);
  }

  uint64_t nextDeadlineMs() const override { return active_ ? offAtMs_ : kNoDeadlineMs; }

 private:
  uint8_t inactiveLevel() const { return activeLevel_ == HIGH ? LOW : HIGH; }                       // Pasif seviye
  void write(bool active) const { digitalWrite(pin_, active ? activeLevel_ : inactiveLevel()); }    // Çıkışı yazar

  uint8_t  pin_;                                          // Çıkış pini
  uint8_t  activeLevel_;                                  // Aktif seviye
  uint32_t durationMs_;                                   // Aktif kalma süresi (ms)
  bool     active_  = false;                              // Şu an aktif mi
  uint64_t offAtMs_ = 0;                                  // Pasife döneceği an
};
