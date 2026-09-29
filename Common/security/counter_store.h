#pragma once

#include <Preferences.h>
#include <array>
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

  std::optional<uint32_t> loadRxCounter(NodeId source) {  // Kaynaktan eyleme dönüşen son mesajın sayacı, hiç yoksa boş
    const RxKey key = rxKey(source);
    if (!preferences_.isKey(key.data())) return std::nullopt;
    return preferences_.getUInt(key.data());
  }

  [[nodiscard]] bool saveRxCounter(NodeId source, uint32_t counter) {  // Kaynaktan eyleme dönüşecek mesajın sayacını kaydeder
    return preferences_.putUInt(rxKey(source).data(), counter) == sizeof(counter);
  }

 private:
  using RxKey = std::array<char, 5>;                      // "rx00".."rx15"

  static constexpr RxKey rxKey(NodeId source) {           // Kaynak başına NVS anahtarı
    return {'r', 'x', static_cast<char>('0' + source / 10), static_cast<char>('0' + source % 10), '\0'};
  }

  static constexpr const char* kNamespace    = "yenizil";    // NVS ad alanı
  static constexpr const char* kTxReserveKey = "txReserve";  // Gönderme sayacı rezervi anahtarı

  Preferences preferences_;                               // NVS erişimi
};
