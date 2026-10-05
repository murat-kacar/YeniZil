#pragma once

#include "frame.h"

namespace yenizil {

class FrameReceiver {                                     // Radyodan gelen çerçeveleri işleyen ağ (Observer): radyo her çerçeveyi bağlı tüm alıcılara verir
 public:
  FrameReceiver() = default;
  FrameReceiver(const FrameReceiver&) = delete;
  FrameReceiver& operator=(const FrameReceiver&) = delete;

  virtual void receive(const frame::Bytes& bytes) = 0;    // Tek çerçeve, döngü görevinde çağrılır

 protected:
  ~FrameReceiver() = default;

 private:
  friend class EspNowRadio;                               // Listeyi radyo kurar
  FrameReceiver* nextReceiver_ = nullptr;                 // Radyonun alıcı listesinde sonraki (intrusive, heap yok)
};

}  // namespace yenizil
