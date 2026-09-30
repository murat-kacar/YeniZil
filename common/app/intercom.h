#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include "../config/radio_settings.h"
#include "../kernel/callback.h"
#include "../kernel/event_loop.h"
#include "../net/burst_sender.h"
#include "../net/esp_now_radio.h"
#include "../net/flood_router.h"
#include "../net/protocol.h"
#include "../security/ccm_cipher.h"
#include "../security/counter_store.h"
#include "../security/secure_channel.h"

namespace yenizil {

struct NetworkSettings {                                  // Ağ ayarları: ünite dosyası verir
  RadioSettings                                 radio;    // Radyo ayarları
  BurstSettings                                 burst;    // Tekrarlı gönderim ayarları
  ApartmentId                                   apartmentId;  // Ağ (apartman) kimliği
  std::span<const uint8_t, CcmCipher::kKeySize> key;      // Ağ şifresi
  NodeId                                        nodeId;   // Bu ünitenin ağdaki numarası
};

constexpr bool isAssigned(const NetworkSettings& network) {  // Ağ kimliği ve şifre doldurulmuş mu (sıfır değil)
  return network.apartmentId != ApartmentId{0} && std::ranges::any_of(network.key, [](uint8_t byte) { return byte != 0; });
}

class Intercom {                                          // Diyafon: ağ katmanını kurar ve gizler, alan dilinde işlemler sunar (Facade)
 public:
  Intercom(const NetworkSettings& network, EventLoop& loop)
      : radio_(network.radio, loop),
        bursts_(radio_, network.burst),
        cipher_(network.key),
        channel_(cipher_, store_, network.apartmentId),
        router_(radio_, bursts_, channel_, network.nodeId) {}

  void ringFlat(NodeId flat) { router_.send(flat, MessageType::kRingBell); }                       // Dairenin zilini çaldırır
  void requestDoorOpen() { router_.send(kOutdoorUnitId, MessageType::kOpenDoor); }                 // Dış üniteden kapıyı açmasını ister
  void broadcastHeartbeat() { router_.send(kAllUnitsId, MessageType::kHeartbeat); }                // Dış ünite: ağda olduğunu duyurur
  void onRing(Handler<> handler) { router_.on(MessageType::kRingBell, handler); }                  // Zil isteği gelince
  void onDoorOpenRequest(Handler<> handler) { router_.on(MessageType::kOpenDoor, handler); }       // Kapı açma isteği gelince
  void onHeartbeat(Handler<> handler) { router_.on(MessageType::kHeartbeat, handler); }            // Dış üniteden "buradayım" gelince

 private:
  EspNowRadio   radio_;                                   // Radyo, bileşen sırası: önce radyo başlar
  BurstSender   bursts_;                                  // Tekrarlı gönderim
  CcmCipher     cipher_;                                  // AES-CCM
  CounterStore  store_;                                   // Sayaç kalıcı kaydı
  SecureChannel channel_;                                 // Çerçeve üretimi ve doğrulama
  FloodRouter   router_;                                  // Flooding, radyodan sonra başlar: MAC'i radyo açılınca okur
};

}  // namespace yenizil
