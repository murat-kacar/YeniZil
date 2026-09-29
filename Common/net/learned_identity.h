#pragma once

#include <Preferences.h>
#include <optional>
#include "../kernel/clock.h"
#include "../kernel/component.h"
#include "node_identity.h"
#include "nodes.h"

class LearnedIdentity final : public Component, public NodeIdentity {  // Eşleştirmeyle öğrenilen kimlik (learn mode): iç ünite, NVS'de saklanır
 public:
  explicit LearnedIdentity(uint32_t pairingWindowMs) : pairingWindowMs_(pairingWindowMs) {}

  void begin() override {                                 // Kayıtlı kimliği yükler, yoksa ya da geçersizse eşleşmemiş sayılır
    if (!preferences_.begin(kNamespace, false) || !preferences_.isKey(kIdKey)) return;
    const NodeId stored = preferences_.getUChar(kIdKey);
    if (isFlatId(stored)) id_ = stored;
  }

  void update(uint64_t nowMs) override {                  // Süre dolunca eşleştirme modundan çıkar
    if (pairing_ && nowMs >= pairingEndMs_) pairing_ = false;
  }

  uint64_t nextDeadlineMs() const override { return pairing_ ? pairingEndMs_ : kNoDeadlineMs; }

  void startPairing() {                                   // Eşleştirme modunu pencere süresince açar
    pairing_      = true;
    pairingEndMs_ = monotonicMs() + pairingWindowMs_;
  }

  std::optional<NodeId> id() const override { return id_; }
  bool isPairing() const override { return !id_ || pairing_; }  // Kimliksiz ünite eşleşene kadar eşleştirme modunda

  bool adopt(NodeId id) override {                        // Kimliği önce NVS'ye yazar, yazılamazsa kabul etmez
    if (!isPairing() || !isFlatId(id) || preferences_.putUChar(kIdKey, id) != sizeof(id)) return false;
    id_      = id;
    pairing_ = false;
    return true;
  }

 private:
  static constexpr const char* kNamespace = "identity";   // NVS ad alanı
  static constexpr const char* kIdKey     = "flatId";     // Daire kimliği anahtarı

  uint32_t              pairingWindowMs_;                 // Eşleştirme modunun süresi (ms)
  std::optional<NodeId> id_;                              // Kimlik, eşleşmemişse boş
  bool                  pairing_      = false;            // Elle açılmış eşleştirme modu
  uint64_t              pairingEndMs_ = 0;                // Eşleştirme modunun bittiği an
  Preferences           preferences_;                     // NVS erişimi
};
