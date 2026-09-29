#pragma once

#include <cstdint>

namespace yenizil {

class ReplayWindow {                                      // Tek kaynak için tekrar penceresi: aynı sayaç ikinci kez kabul edilmez (RFC 4303 / RFC 6347)
 public:
  static constexpr uint32_t kSize = 64;                   // Pencere boyutu: RFC 6347 varsayılanı

  [[nodiscard]] constexpr bool isFresh(uint32_t counter) const {  // Sayaç daha önce görülmedi ve pencerenin gerisinde değil mi
    if (!started_ || counter > highest_) return true;
    const uint32_t offset = highest_ - counter;
    return offset < kSize && (seen_ & (uint64_t{1} << offset)) == 0;
  }

  constexpr void markSeen(uint32_t counter) {             // Sayacı görüldü olarak işaretler, isFresh() true olduktan sonra çağrılır
    if (!started_) {
      started_ = true;
      highest_ = counter;
      seen_    = 1;
      return;
    }
    if (counter > highest_) {
      const uint32_t shift = counter - highest_;
      seen_    = shift >= kSize ? 0 : seen_ << shift;
      seen_   |= 1;
      highest_ = counter;
      return;
    }
    seen_ |= uint64_t{1} << (highest_ - counter);
  }

  constexpr void restore(uint32_t counter) {              // Kayıtlı sayaca kadar hepsini görülmüş sayar: yeniden başlamadan sonra kaydedilmiş eski çerçeve kabul edilmez
    started_ = true;
    highest_ = counter;
    seen_    = ~uint64_t{0};
  }

 private:
  bool     started_ = false;                              // En az bir sayaç görüldü mü
  uint32_t highest_ = 0;                                  // Görülen en yüksek sayaç
  uint64_t seen_    = 0;                                  // Bit i: highest_ - i görüldü
};

}  // namespace yenizil
