#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace yenizil {

enum class NodeId : uint8_t {};                           // Ünite kimliği, güçlü tip (std::byte kalıbı): 0 kapı ünitesi, daireler 1..kFlatCount
enum class NetworkId : uint32_t {};                       // Ağ kimliği, güçlü tip: bina ağı ve zil paneli bağlantısı ayrı
enum class FrameCounter : uint32_t {};                    // Gönderenin çerçeve sayacı, güçlü tip

using MacAddress = std::array<uint8_t, 6>;                // MAC adresi

inline constexpr NodeId  kDoorUnitId{0};                  // Kapı ünitesi (dış2) kimliği, daireler 1'den başlar
inline constexpr NodeId  kBellPanelId{0xFE};              // Zil paneli (dış1) kimliği: sadece zil paneli bağlantısında
inline constexpr NodeId  kAllUnitsId{0xFF};               // Herkese: tüm üniteler teslim alır ve aktarır
inline constexpr uint8_t kProtocolVersion = 2;            // Çerçeve biçimi sürümü

enum class MessageType : uint8_t {                        // Mesaj tipi
  kRingBell  = 1,                                         // Zil çal
  kOpenDoor  = 2,                                         // Kapıyı aç
  kHeartbeat = 3,                                         // Kapı ünitesi "buradayım" yayını, herkese
};

inline constexpr std::size_t kMessageTypeCount = 4;       // Tip değerleriyle indekslenen tabloların boyutu, 0 kullanılmaz

constexpr bool isKnownMessageType(uint8_t value) {        // Değer tanımlı bir mesaj tipi mi
  return value >= 1 && value < kMessageTypeCount;
}

constexpr bool isBroadcast(MessageType type) {            // Tip herkese mi gider: eylem değildir, kalıcı kayıt gerektirmez
  return type == MessageType::kHeartbeat;
}

constexpr bool matchesAddressing(MessageType type, NodeId destination) {  // Herkese giden tip sadece herkese, diğerleri sadece tek üniteye gider
  return isBroadcast(type) == (destination == kAllUnitsId);
}

struct Message {                                          // Doğrulanmış mesaj
  MacAddress   sourceMac;                                 // Gönderen kartın MAC adresi: tekrar koruması buna göre
  NodeId       source;                                    // Gönderen ünite
  NodeId       destination;                               // Hedef ünite
  FrameCounter counter;                                   // Gönderenin sayacı
  MessageType  type;                                      // Mesaj tipi
};

}  // namespace yenizil
