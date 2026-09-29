#pragma once

#include <Preferences.h>
#include <cstdint>

class CounterStore {                                      // Sayaçların flash'ta (NVS) kalıcı kaydı
 public:
  void begin() { preferences_.begin(kNamespace, false); }

  uint32_t loadTxReserve() { return preferences_.getUInt(kTxReserveKey, 0); }             // Kayıtlı gönderme sayacı rezervi, ilk açılışta 0
  void saveTxReserve(uint32_t value) { preferences_.putUInt(kTxReserveKey, value); }      // Gönderme sayacı rezervini kaydeder

 private:
  static constexpr const char* kNamespace    = "yenizil";   // NVS ad alanı
  static constexpr const char* kTxReserveKey = "txReserve"; // Gönderme sayacı rezervi anahtarı

  Preferences preferences_;                               // NVS erişimi
};
