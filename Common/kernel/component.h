#pragma once

#include <stdint.h>

inline constexpr uint64_t kNoDeadlineMs = UINT64_MAX;  // Zamanlanmış iş yok

class Component {                                                     // Olay döngüsünün çalıştırdığı bileşen
 public:
  Component() { append(this); }                                       // Tanımlanma sırasıyla listeye eklenir
  Component(const Component&) = delete;
  Component& operator=(const Component&) = delete;
  virtual ~Component() = default;

  virtual void begin() {}                                             // Donanımı hazırlar, EventLoop::begin() çağırır
  virtual void update(uint64_t nowMs) = 0;                            // Zamanı gelen işi yapar
  virtual uint64_t nextDeadlineMs() const { return kNoDeadlineMs; }   // Bir sonraki işin zamanı

  static Component* first() { return head_; }                         // Listedeki ilk bileşen
  Component* next() const { return next_; }                           // Listedeki sonraki bileşen

 private:
  static void append(Component* component) {                          // Listenin sonuna ekler, heap kullanmaz
    if (tail_ != nullptr) tail_->next_ = component;
    else head_ = component;
    tail_ = component;
  }

  static inline Component* head_ = nullptr;                           // İlk bileşen
  static inline Component* tail_ = nullptr;                           // Son bileşen
  Component* next_ = nullptr;                                         // Sonraki bileşen
};
