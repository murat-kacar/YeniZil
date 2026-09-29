#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <algorithm>
#include <cstdint>
#include "clock.h"
#include "component.h"

namespace yenizil {

class EventLoop {                                                        // Bileşenleri çalıştıran olay döngüsü, tek örneği ünite dosyasında
 public:
  static constexpr uint32_t kMaxWaitMs = 1000;                           // En uzun bekleme (ms), watchdog süresinin (5 sn) altında

  void begin() {
    task_ = xTaskGetCurrentTaskHandle();
    for (Component* component = Component::first(); component != nullptr; component = component->next()) component->begin();
    enableLoopWDT();                                                     // loop() takılırsa cihaz yeniden başlar
  }

  void update() {
    const uint64_t nowMs = monotonicMs();
    for (Component* component = Component::first(); component != nullptr; component = component->next()) component->update(nowMs);
    uint64_t deadlineMs = nowMs + kMaxWaitMs;                            // Zamanlar tüm güncellemelerden sonra toplanır: bir bileşen diğerine iş verebilir
    for (Component* component = Component::first(); component != nullptr; component = component->next()) deadlineMs = std::min(deadlineMs, component->nextDeadlineMs());
    waitUntil(deadlineMs);
  }

  void notify() {                                                        // Başka bir görevden döngüyü hemen uyandırır
    if (task_ != nullptr) xTaskNotifyGive(task_);
  }

 private:
  void waitUntil(uint64_t deadlineMs) const {                            // Zamana ya da bildirime kadar bekler, işlemci bu sürede WFI ile boşta
    const uint64_t nowMs = monotonicMs();
    if (deadlineMs <= nowMs) return;
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(static_cast<uint32_t>(deadlineMs - nowMs)));
  }

  TaskHandle_t task_ = nullptr;                                          // loop() görevinin tanıtıcısı
};

}  // namespace yenizil
