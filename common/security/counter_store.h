#pragma once

#include <Preferences.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "../net/protocol.h"

class CounterStore {                                      // Sayaçların flash'ta (NVS) kalıcı kaydı: Preferences
 public:
  [[nodiscard]] bool begin() { return preferences_.begin(kNamespace, false); }  // NVS ad alanını açar, başarısızsa false

  uint32_t loadTxReserve() { return preferences_.getUInt(kTxReserveKey, 0); }  // Kayıtlı gönderme sayacı rezervi, ilk açılışta 0

  [[nodiscard]] bool saveTxReserve(uint32_t value) {      // Gönderme sayacı rezervini kaydeder
    return preferences_.putUInt(kTxReserveKey, value) == sizeof(value);
  }

  std::optional<uint32_t> loadRxCounter(const MacAddress& source) {  // Gönderen karttan eyleme dönüşen son mesajın sayacı, hiç yoksa boş
    const RxKey key = rxKey(source);
    if (!preferences_.isKey(key.data())) return std::nullopt;
    return preferences_.getUInt(key.data());
  }

  [[nodiscard]] bool saveRxCounter(const MacAddress& source, uint32_t counter) {  // Gönderen karttan eyleme dönüşecek mesajın sayacını kaydeder
    return preferences_.putUInt(rxKey(source).data(), counter) == sizeof(counter);
  }

 private:
  using RxKey = std::array<char, 15>;                     // "rx" + MAC'in 12 onaltılık hanesi + '\0', NVS sınırı 15 karakter

  static constexpr RxKey rxKey(const MacAddress& source) {  // Gönderen kart başına NVS anahtarı
    constexpr char kHex[] = "0123456789abcdef";
    RxKey key{'r', 'x'};
    for (std::size_t i = 0; i < source.size(); ++i) {
      key[2 + 2 * i] = kHex[source[i] >> 4];
      key[3 + 2 * i] = kHex[source[i] & 0x0F];
    }
    return key;
  }

  static constexpr const char* kNamespace    = "yenizil";    // NVS ad alanı
  static constexpr const char* kTxReserveKey = "txReserve";  // Gönderme sayacı rezervi anahtarı

  Preferences preferences_;                               // NVS erişimi
};
