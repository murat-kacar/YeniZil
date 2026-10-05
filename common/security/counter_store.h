#pragma once

#include <Preferences.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "../net/protocol.h"

namespace yenizil {

class CounterStore {                                      // Sayaçların flash'ta (NVS) kalıcı kaydı: Preferences
 public:
  [[nodiscard]] bool begin() {                            // NVS ad alanını açar, başarısızsa false. Sayaç ve ağlar ayrı ayrı çağırır, ikinci çağrı açık olanı kullanır
    if (!started_) started_ = preferences_.begin(kNamespace, false);
    return started_;
  }

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
  static constexpr std::size_t kRxPrefixSize = 2;         // "rx" ön eki
  static constexpr std::size_t kNvsKeyMax    = 15;        // NVS anahtarının en fazla karakter sayısı

  using RxKey = std::array<char, kRxPrefixSize + 2 * std::tuple_size_v<MacAddress> + 1>;  // "rx" + MAC'in onaltılık haneleri + '\0'
  static_assert(std::tuple_size_v<RxKey> - 1 <= kNvsKeyMax, "NVS anahtarı en fazla 15 karakter olabilir");

  static constexpr RxKey rxKey(const MacAddress& source) {  // Gönderen kart başına NVS anahtarı
    constexpr char kHex[] = "0123456789abcdef";
    RxKey key{'r', 'x'};
    for (std::size_t i = 0; i < source.size(); ++i) {
      key[kRxPrefixSize + 2 * i]     = kHex[source[i] >> 4];
      key[kRxPrefixSize + 2 * i + 1] = kHex[source[i] & 0x0F];
    }
    return key;
  }

  static constexpr const char* kNamespace    = "yenizil";    // NVS ad alanı
  static constexpr const char* kTxReserveKey = "txReserve";  // Gönderme sayacı rezervi anahtarı

  Preferences preferences_;                               // NVS erişimi
  bool        started_ = false;                           // Ad alanı açık mı
};

}  // namespace yenizil
