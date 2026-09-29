#pragma once

namespace yenizil {

template <typename... Args>
using Handler = void (*)(Args...);                        // Olay işleyicisi: değişken yakalamayan lambda ya da fonksiyon, heap kullanmaz

template <typename... Args>
constexpr void callIfSet(Handler<Args...> handler, Args... args) {  // İşleyici bağlıysa çağırır
  if (handler != nullptr) handler(args...);
}

}  // namespace yenizil
