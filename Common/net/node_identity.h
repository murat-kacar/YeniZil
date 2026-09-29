#pragma once

#include <optional>
#include "protocol.h"

class NodeIdentity {                                      // Bu ünitenin ağdaki mantıksal adresi (Strategy arayüzü)
 public:
  virtual ~NodeIdentity() = default;

  virtual std::optional<NodeId> id() const = 0;           // Kimlik, eşleşmemişse boş
  virtual bool isPairing() const = 0;                     // Eşleştirme modunda mı
  virtual bool adopt(NodeId id) = 0;                      // Eşleştirmede gelen kimliği kabul eder, başarısızsa false
};

class FixedIdentity final : public NodeIdentity {         // Değişmeyen kimlik: dış ünite
 public:
  explicit FixedIdentity(NodeId id) : id_(id) {}

  std::optional<NodeId> id() const override { return id_; }
  bool isPairing() const override { return false; }
  bool adopt(NodeId) override { return false; }

 private:
  NodeId id_;                                             // Kimlik
};
