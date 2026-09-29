#pragma once

#include <cstddef>

template <typename T, std::size_t N, typename Key>
consteval bool allUnique(const T (&items)[N], Key key) {  // Tablodaki anahtarlar tekrarsız mı, derleme zamanında
  for (std::size_t i = 0; i < N; ++i)
    for (std::size_t j = i + 1; j < N; ++j)
      if (key(items[i]) == key(items[j])) return false;
  return true;
}
