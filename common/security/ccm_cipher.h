#pragma once

#include <mbedtls/ccm.h>
#include <cstddef>
#include <cstdint>
#include <span>

class CcmCipher {                                         // AES-128-CCM şifreleme ve doğrulama: mbedTLS, C3'te donanım AES ile
 public:
  static constexpr std::size_t kKeySize = 16;             // Anahtar boyutu (bayt): AES-128

  explicit CcmCipher(std::span<const uint8_t, kKeySize> key) : key_(key) {}
  CcmCipher(const CcmCipher&) = delete;
  CcmCipher& operator=(const CcmCipher&) = delete;
  ~CcmCipher() { mbedtls_ccm_free(&context_); }

  [[nodiscard]] bool begin() {                            // Anahtarı yükler, başarısızsa false
    mbedtls_ccm_init(&context_);
    ready_ = mbedtls_ccm_setkey(&context_, MBEDTLS_CIPHER_ID_AES, key_.data(), kKeySize * 8) == 0;
    return ready_;
  }

  [[nodiscard]] bool seal(std::span<const uint8_t> nonce, std::span<const uint8_t> header, std::span<const uint8_t> plain,
                          std::span<uint8_t> encrypted, std::span<uint8_t> tag) {  // Veriyi şifreler, başlık ve veri için etiket üretir
    if (!ready_ || encrypted.size() != plain.size()) return false;
    return mbedtls_ccm_encrypt_and_tag(&context_, plain.size(), nonce.data(), nonce.size(), header.data(), header.size(), plain.data(),
                                       encrypted.data(), tag.data(), tag.size()) == 0;
  }

  [[nodiscard]] bool open(std::span<const uint8_t> nonce, std::span<const uint8_t> header, std::span<const uint8_t> encrypted,
                          std::span<const uint8_t> tag, std::span<uint8_t> plain) {  // Etiketi doğrular ve şifreyi çözer, tek bit değişse false
    if (!ready_ || plain.size() != encrypted.size()) return false;
    return mbedtls_ccm_auth_decrypt(&context_, encrypted.size(), nonce.data(), nonce.size(), header.data(), header.size(), encrypted.data(),
                                    plain.data(), tag.data(), tag.size()) == 0;
  }

 private:
  std::span<const uint8_t, kKeySize> key_;                // Ağ şifresi, ünitenin config dosyasında
  mbedtls_ccm_context                context_{};          // mbedTLS durumu
  bool                               ready_ = false;      // Anahtar yüklendi mi
};
