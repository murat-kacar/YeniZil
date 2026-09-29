# YeniZil — Mimari

Sürüm 2 · 29.09.2026 · Durum: **kod tamam, donanım kurulumu bekliyor**

---

## 1. Kapsam

### 1.1 Gereksinimler

- 4 katlı bina, her katta 1 daire.
- **Dış ünite:** 4 zil butonu + kapı rölesi tetiği (3.3V).
- **İç ünite (×4):** "kapıyı aç" butonu + zil tetiği (3.3V) + bağlantı LED'i.
- Her ünite kendi 5V adaptöründen beslenir. Pil yok.
- Tamamen kablosuz, internet yok. ESP-NOW ile flooding.
- Kart: ESP32-C3 Super Mini. Derleme ortamı: Arduino IDE, esp32 core 3.3.11 (ESP-IDF 5.5.5).

| Tetikleyici | Sonuç |
|---|---|
| Dış ünitede N. butona basılıp bırakıldı | N. dairenin zili 1,5 sn çalar |
| N. dairede "kapıyı aç" butonuna basılıp bırakıldı | Kapı rölesi 1,5 sn tetiklenir |
| İç ünite dış üniteyle bağlantıda | Bağlantı LED'i sürekli yanar, bağlantı yokken yanıp söner |

### 1.2 Kodun kapsamı

Depoda sadece şu beş işi yapan C++ kodu bulunur:

1. **İşin kendisi:** buton okuma, zil ve röle tetiği, ESP-NOW ile mesajlaşma ve aktarma.
2. **Güvenlik:** AES-128-CCM, tekrar koruması, kalıcı sayaçlar.
3. **Güç tasarrufu:** modem uykusu ve uyanma penceresi, 80 MHz CPU, boşta WFI.
4. **Denetimler:** basış kuralları, çerçeve denetimi, ayar ve kablolama için derleme anı `static_assert`'leri.
5. **Optimizasyonlar:** tekrar gelen kopyaların şifre çözülmeden atılması gibi.

Test kodu, tezgâh (bench) programı, yardımcı betik ve seri monitör çıktısı (log) yazılmaz. Derleme, karta yükleme ve deneme Arduino IDE ile kullanıcı tarafından yapılır. Bir sorun çıkarsa ilgili test o zaman ayrıca istenir.

Her şey **Arduino-first**: önce Arduino-ESP32 core'un API ve kütüphaneleri kullanılır. ESP-IDF ya da FreeRTOS çağrısı sadece Arduino karşılığı yoksa kullanılır. Hazır kütüphane yoksa ya da mevcut olanlar güvenilmezse kod elle yazılır. Hangisinin nerede kullanıldığı 5.2'de.

### 1.3 Kapsam dışı (YAGNI)

İhtiyaç doğduğunda eklenecek:

- Bağlantı LED'i dışındaki geri bildirim LED'leri
- "Kapı sadece zil çaldıktan sonra açılabilsin" kuralı
- Zaman senkronizasyonu (FTSP)
- OTA güncelleme ve flash şifreleme
- Otomatik hafif uyku (ESP-IDF'e geçiş gerektirir, bkz. Karar 1)

---

## 2. Kararlar

### Karar 1 — Güç: modem uykusu + uyanma penceresi

Kurulu core'un `sdkconfig` dosyasında:
- `CONFIG_PM_ENABLE` kapalı. Yani işlemci boştayken otomatik uykuya geçemiyor.
- `CONFIG_ESP_WIFI_STA_DISCONNECTED_PM_ENABLE=y`. Yani ESP-NOW uyanma penceresi çalışıyor.

Beklenen tüketim (C3 datasheet: alım 84 mA, 80 MHz'de modem-sleep ve işlemci boşta 13–18 mA):

| Mod | Ortalama akım | Güç (≈) |
|---|---|---|
| Sürekli dinleme | ≈ 85–100 mA | ≈ 0,45 W |
| %10 pencere | 0,1 × 84 + 0,9 × 13–18 ≈ **22–25 mA** | ≈ **0,12 W** |

**Karar:** Radyo modem uykusunda. 200 ms'de bir 20 ms uyanıp dinliyor. İşlemci boşta WFI ile bekliyor. Tüketim yaklaşık 3,5 kat düşüyor ve ısınma ihmal edilebilir hale geliyor.

### Karar 2 — Dış ünite ESP'si bina içinde

Anahtar ve röle tetik kablosu bina dışındaki kutuda durursa kutuyu açan biri tetik kablosunu 3.3V'a değdirip kapıyı açabilir. Ya da flash'ı okuyup anahtarı alabilir.

**Karar:** ESP ve röle tetiği bina içinde, kapının güvenli tarafında olacak. Dışarıda sadece butonlar kalacak. Bu, erişim kontrol sistemlerinde yaygın bir kurulum kuralı. Aynı karar yağmur, güneş ve ısınma sorununu da çözüyor.

### Karar 3 — Alıcı sayacı kalıcı

Tekrar koruma durumu yalnızca RAM'de tutulursa şu saldırı mümkün olur:
1. Saldırgan bir "kapıyı aç" mesajını havadan kaydeder.
2. Dış ünitenin fişini çekip takar. Tekrar koruma durumu sıfırlanır.
3. Kaydettiği mesajı tekrar gönderir, kapı açılır.

**Karar:** Kendine gelen ve eyleme dönüşecek her mesajın sayacı, gönderen kartın MAC'i anahtar olarak kullanılarak **eylemden önce** NVS'ye yazılıyor. Yazılamazsa eylem yapılmıyor. Açılıştan sonra bir karttan ilk doğrulanmış çerçeve geldiğinde o kartın tekrar penceresi "kayıtlı sayaca kadar hepsi görüldü" durumuyla başlıyor. NVS aşınmayı dengelediği için flash ömrü sorun değil: günde 50 olay × 5 ünite, 100 bin silme döngüsünün çok altında kalıyor.

### Karar 4 — Kimlik: MAC + eşleştirme (IEEE 802.15.4 / Zigbee / kablosuz zil yöntemi)

Standart kablosuz ağlarda iki kavram ayrı tutuluyor:

- **Cihaz kimliği = fabrika MAC adresi.** Gönderen kartın MAC'i her çerçevede taşınıyor. AES-CCM nonce'u MAC + sayaçtan üretiliyor. Tekrar koruması ve kalıcı sayaçlar göndereni MAC'e göre izliyor. 802.15.4 CCM* nonce'u da gönderen adresi + sayaçtan oluşuyor. MAC fabrikadan tekil olduğu için nonce hiçbir koşulda tekrarlanmıyor. Bir kart değiştirildiğinde yeni kartın sayacı sıfırdan başlasa bile reddedilmiyor.
- **Mantıksal adres (hangi daire) = kurulumda eşleştirme (learn mode).** Dış ünite her zaman 0. İç ünitenin daire numarası firmware'e yazılmıyor, eşleştirmeyle öğrenilip NVS'de saklanıyor:
  1. Eşleşmemiş iç ünite açıldığı andan itibaren eşleştirme modundadır. Daha önce eşleşmiş bir üniteyi yeniden eşleştirmek için "kapıyı aç" butonu basılıyken fişe takılır, en az 5 sn basılı tutulup bırakılır. Eşleştirme modu 2 dakika sürer.
  2. Dış ünitede o dairenin butonuna basılır.
  3. Eşleştirme modundaki ünite doğrulanmış ilk zil isteğinin dairesini kendi kimliği olarak NVS'ye yazar ve zili çalar. Zilin çalması eşleşmenin onayıdır.

Eşleştirme mesajları da apartman anahtarıyla doğrulanıyor. Anahtarı bilmeyen biri bir üniteye kimlik atayamaz. Eşleştirme sırasında başka bir dairenin zili çalınırsa ünite o daireyi öğrenir. Bu durumda eşleştirme tekrarlanır.

### Karar 5 — TTL yok

Tekrar koruması her kaynak için kayan pencereyle çalışıyor. Bu yüzden bir düğüm aynı mesajı en fazla bir kez aktarıyor ve flooding'in sonlanması garanti. `hopLimit` alanı yok, bu sayede başlığın tamamı imzalı.

### Karar 6 — Sabit sıralı işlem hattı

Güvenlik adımlarının sırası kritik. Pencere doğrulamadan önce ilerletilirse sahte yüksek sayaçlar gerçek mesajları engelleyebilir (DoS).

**Karar:** Adımlar `SecureChannel` içinde sabit sırayla çalışıyor (Pipes and Filters kalıbının statik biçimi). Her adım ayrı bir sınıf: `frame`, `CcmCipher`, `ReplayWindow`, `CounterStore`. Sıra çalışma anında değiştirilemiyor.

### Karar 7 — Basış kuralları tek yapıda

Buton denetimlerinin hepsi bir basışın sayısal sınırları: en kısa süre, en uzun süre, bekleme süresi. Dış ünitedeki daire başına zil beklemesi ile iç ünitedeki kapı isteği beklemesi aynı ihtiyaç.

**Karar:** Hepsi `PressConfig` yapısında. Ayrı kural sınıfları yok (Rule of Three), aynı mekanizma iki ünitede de kullanılıyor (DRY).

### Karar 8 — 64-bit zaman

`millis()` 49,7 günde taşar. **Karar:** Zaman `esp_timer` tabanlı 64-bit `monotonicMs()` fonksiyonundan alınıyor.

### Karar 9 — Butonlar örneklemeyle okunuyor

**Karar:** Butonlar 5 ms'de bir okunuyor (Ganssle: 1–5 ms). İşlemci otomatik uykuya geçmediği için (Karar 1) ek maliyeti yok. Kesme ve uyku seviyesi değişikliğinden gelen karmaşıklık da ortadan kalkıyor.

### Karar 10 — Burst süresi

Tek çerçevenin bir kat geçişinde ulaşma oranı %90 ise, alıcının 1 pencerede 2 kopya gördüğü durumda kat başına kaçırma %1, 4 katta ≈ %4 olur.

**Karar:** Burst süresi = 2 × aralık + pencere = **420 ms**. Alıcı 2 pencere, her pencerede 2 kopya görüyor. Kaçırma kat başına %0,01, 4 katta ≈ %0,04. Gecikme değişmiyor, çünkü mesaj yine ilk pencerede yakalanıyor.

### Karar 11 — Sırlar git dışında

**Karar:** Siteye özgü değerler (apartman kimliği, anahtar, daire sayısı, kanal, TX gücü) git dışındaki `site_config.h` dosyasında. `site_config.example.h` şablon. Kimlik ya da anahtar sıfır bırakılırsa program derlenmiyor.

### Karar 12 — AES-128-CCM, 8 bayt etiket

IEEE 802.15.4, Zigbee, Thread ve BLE'nin kullandığı standart. mbedTLS core'da hazır ve C3'te donanım hızlandırmalı: `CONFIG_MBEDTLS_HARDWARE_AES=y`, `CONFIG_MBEDTLS_CCM_C=y`. Kapı açma kritik olduğu için etiket 4 değil 8 bayt.

### Karar 13 — Gönderme sayacı rezervi ve sınırı

Gönderici her mesajda sayacı flash'a yazmıyor. 1000'lik bir rezervin sonunu yazıyor, yeniden başlayınca oradan devam ediyor (OpenThread `STORE_FRAME_COUNTER_AHEAD`). Rezerv flash'a yazılamazsa ya da sayaç 32 bitin sonuna gelirse gönderim duruyor. Nonce tekrarlanmasın diye bu durumda anahtarın değişmesi gerekiyor. Aynı nedenle Arduino IDE'de "Erase All Flash Before Sketch Upload" kapalı kalmalı: açılırsa kayıtlı sayaçlar silinir, anahtar da değiştirilmelidir.

### Karar 14 — Radyo ayarları radyonun işi

TX gücü, Long Range, kanal, uyanma aralığı ve penceresi `RadioConfig` içinde, `EspNowRadio` tarafından uygulanıyor. `PowerManager` sadece CPU frekansını yönetiyor. Böylece bileşenler arasında başlatma sırası bağımlılığı kalmıyor.

### Karar 15 — Watchdog

`CONFIG_ESP_TASK_WDT_TIMEOUT_S=5`. **Karar:** Döngü görevi watchdog'a bağlı ve olay beklemesi en fazla 1 sn. Kod kilitlenirse cihaz yeniden başlıyor. Röle pini pull-down'da olduğu için yeniden başlama kapıyı açmıyor.

### Karar 16 — Kopyalar şifre çözülmeden atılıyor (optimizasyon)

Her mesaj burst yüzünden ~40 kopya geliyor. Tekrar penceresi salt okunur olarak AES'ten **önce** kontrol ediliyor, görülmüş kopya şifre çözülmeden atılıyor. Pencere sadece doğrulanmış çerçeveyle ilerlediği için bu sıralama Karar 6'daki DoS riskini doğurmuyor.

### Karar 17 — Log yok

**Karar:** Kodda seri port ve log yok. Hatalar dönüş değeriyle bildiriliyor, başlayamayan bileşen kapalı kalıyor. Örneğin NVS açılamazsa ya da anahtar yüklenemezse ağ açılmıyor.

### Karar 18 — Bağlantı göstergesi: heartbeat

Flooding ağında sürekli bir bağlantı yok, mesaj sadece olay olunca gidiyor. İç ünitenin "bağlıyım" diyebilmesi için düzenli bir sinyal gerekiyor.

**Karar:** Dış ünite her 30 sn'de bir, açılışta da hemen, herkese (`kAllUnitsId`) doğrulanmış bir heartbeat yayınlıyor. Her ünite bunu teslim alıp aktarıyor, böylece üst katlara da ulaşıyor. İç ünite 95 sn (3 kaçırılan yayın + pay) boyunca heartbeat alamazsa bağlantıyı kopmuş sayıyor. Bağlantı LED'i bağlıyken sürekli yanıyor, bağlantı yokken yanıp sönüyor. Heartbeat bir eylem değil. Bu yüzden sayacı flash'a yazılmıyor, flash aşınmıyor. Maliyeti her ünitenin 30 sn'de bir 420 ms'lik bir burst göndermesi (radyo gönderimi %1,4).

---

## 3. Mimari

### 3.1 Katmanlar

```
┌─────────────────────────────────────────────────────────┐
│ Sketch     *.ino               bağlamalar: aksiyon → sonuç │
├─────────────────────────────────────────────────────────┤
│ Unit       outdoor_unit.h      composition root:          │
│            indoor_unit.h       nesneleri kurar, ayar verir │
├─────────────────────────────────────────────────────────┤
│ App        Intercom · Bell · DoorOpener      alan dili    │
├─────────────────────────────────────────────────────────┤
│ Services   FloodRouter · SecureChannel                    │
│            ButtonGroup · Button · PulseOutput             │
├─────────────────────────────────────────────────────────┤
│ Platform   EspNowRadio · BroadcastPeer · CcmCipher        │
│            CounterStore · PowerManager                    │
│            (ESP-IDF / Arduino'ya dokunan tek katman)       │
├─────────────────────────────────────────────────────────┤
│ Core       PressDetector · ReplayWindow · frame · protocol │
│            (saf C++: Arduino/IDF include etmez)            │
├─────────────────────────────────────────────────────────┤
│ Kernel     EventLoop · Component · monotonicMs()          │
└─────────────────────────────────────────────────────────┘
```

Kurallar:
- Bağımlılık sadece aşağı doğru olabilir.
- Config her katmana değer verir ama hiçbir katmana bağımlı değildir.
- Core katmanındaki dosyalar platforma dokunmaz.

### 3.2 Klasör yapısı (özelliğe göre paketleme)

```
YeniZil/
├── OutDoor/
│   ├── OutDoor.ino           bağlamalar
│   ├── outdoor_unit.h        composition root
│   ├── hardware.h            kablolama: buton ve röle pinleri
│   └── unit_config.h         dış üniteye özel ayarlar
├── InDoor/
│   ├── InDoor.ino
│   ├── indoor_unit.h
│   ├── hardware.h
│   └── unit_config.h
├── Common/
│   ├── config/
│   │   ├── site_config.example.h   şablon (git'te)
│   │   ├── site_config.h           gerçek değerler (git dışı)
│   │   ├── radio_config.h          kanal, güç, pencere, aralık, burst
│   │   ├── input_config.h          basış sınırları
│   │   ├── link_config.h           heartbeat aralığı, bağlantı zaman aşımı
│   │   └── power_config.h          CPU frekansı
│   ├── kernel/   clock.h · component.h · polling_component.h · periodic_timer.h · event_loop.h · byte_order.h · static_checks.h
│   ├── io/       board_pins.h · digital_pin.h · press_detector.h · button.h · button_group.h · pulse_output.h · indicator_led.h
│   ├── net/      protocol.h · frame.h · nodes.h · node_identity.h · learned_identity.h · broadcast_peer.h · esp_now_radio.h · flood_router.h
│   ├── security/ ccm_cipher.h · replay_window.h · counter_store.h · secure_channel.h
│   ├── power/    power_manager.h
│   └── app/      intercom.h · bell.h · door_opener.h · link_monitor.h
├── docs/ARCHITECTURE.md
└── .gitignore                      site_config.h
```

### 3.3 Sınıflar

| Sınıf | Tür | Sorumluluk | Public arayüz |
|---|---|---|---|
| `monotonicMs()` | kernel | 64-bit monoton zaman (ms), `esp_timer` | `uint64_t monotonicMs()` |
| `Component` | soyut | Güncellenen her şeyin ortak arayüzü. Constructor'da kendini zincire ekler (intrusive list, heap yok). | `begin()`, `update(nowMs)`, `nextDeadlineMs()` |
| `PollingComponent` | soyut | Sabit periyotla örnekleme (Template Method). `Button` ve `ButtonGroup` ortak zamanlamayı buradan alır. | `poll(nowMs)` (korumalı) |
| `PeriodicTimer` | kernel | Sabit aralıkla olay, ilk olay açılışta (`PollingComponent`) | `onTick(void(*)())` |
| `EventLoop` | kernel | Bileşenleri başlatır ve günceller. En yakın zamana ya da bildirime kadar bloklanır (≤ 1 sn). Watchdog'u açar. | `begin()`, `update()`, `notify()` |
| `writeLe` / `readLe` | kernel | Tamsayıyı little-endian yazar/okur (`std::bit_cast`) | — |
| `allUnique()` | kernel | Tabloda tekrar var mı, derleme zamanında | `consteval bool allUnique(items, key)` |
| `board::isSafeGpio()` | io | Super Mini'de açılışı etkilemeyen pinler | `constexpr bool isSafeGpio(pin)` |
| `PressDetector` | core | Seviye ve zamandan basış olayı çıkarır: süre sınırları, bekleme süresi, açılışta basılı butonu bırakılana kadar yok sayma, açılışta uzun basılı tutma | `PressEvent update(pressed, nowMs, config)` |
| `Button` | servis | Tek buton: 5 ms örnekleme + `PressDetector` | `onPress(void(*)())`, `onStartupHold(void(*)())` |
| `ButtonGroup<N>` | servis | Kimlikli N buton, olay kimlikle gelir | `onPress(void(*)(uint8_t))` |
| `PulseOutput` | servis | Belirli süre aktif kalan çıkış. Aktifken gelen tetik yok sayılır. Açılışta titremeden pasife çekilir. | `activate()` |
| `IndicatorLed` | servis | Sürekli yanan ya da yanıp sönen LED, pini sadece durum değişince yazar | `turnOn()`, `blink()` |
| `LinkMonitor` | app | Süre içinde heartbeat geldiyse bağlı, gelmezse koptu. Açılışta kopuk. | `refresh()`, `onConnected()`, `onLost()` |
| `Bell` / `DoorOpener` | app | Alan dilinde eylem (`PulseOutput` içerir) | `ring()` / `open()` |
| `protocol` | core | `NodeId`, `MacAddress`, `MessageType`, `Message`, sürüm | `isKnownMessageType()` |
| `frame` | core | Çerçeveyi bayt bayt yazar ve okur, nonce üretir, alanlara `std::span` verir | `encodeHeader()`, `decodeHeader()`, `nonce()`, `header()`, `payload()`, `tag()` |
| `nodes` | denetim | Ünite sayısı, daire kimliği ve apartman kimliği denetimleri | `kNodeCount`, `isFlatId()` |
| `NodeIdentity` | arayüz (Strategy) | Bu ünitenin mantıksal adresi | `id()`, `isPairing()`, `adopt(id)` |
| `FixedIdentity` | core | Sabit kimlik: dış ünite (0) | — |
| `LearnedIdentity` | platform | Eşleştirmeyle öğrenilen, NVS'de saklanan kimlik: iç ünite | `startPairing()` |
| `BroadcastPeer` | platform | Arduino `ESP_NOW_Peer`'den türeyen yayın eşi, Long Range hızıyla | `begin()`, `send(bytes)` |
| `EspNowRadio` | platform | Wi-Fi/ESP-NOW başlatma, kanal, TX gücü, Long Range, modem uykusu ve uyanma penceresi, burst gönderim, alma kuyruğu | `broadcast(bytes)`, `relay(bytes)`, `bool receive(frame)`, `ownMac()` |
| `CcmCipher` | platform | AES-128-CCM (mbedTLS) | `bool begin()`, `bool seal(...)`, `bool open(...)` |
| `ReplayWindow` | core | 64'lük kayan pencere | `isFresh()`, `markSeen()`, `restore()` |
| `CounterStore` | platform | NVS (`Preferences`): gönderme sayacı rezervi, gönderen MAC başına son eylem sayacı | `begin()`, `loadTxReserve()`, `saveTxReserve()`, `loadRxCounter()`, `saveRxCounter()` |
| `SecureChannel` | servis | Sabit sıra (3.6). Giden çerçeveyi şifreler ve imzalar. Tekrar penceresi gönderen MAC başına. | `bool begin(mac)`, `optional<Bytes> seal(src, dst, type)`, `optional<Message> open(bytes)`, `bool commit(message)` |
| `FloodRouter` | servis | Gelen çerçeveyi süzer, eşleştirme modunda kimliği öğretir, kendine geleni kalıcı kayıttan sonra teslim eder, gerisini **değiştirmeden** aktarır | `send(dst, type)`, `on(type, void(*)())` |
| `Intercom` | app (Facade) | Protokolü gizler, alan dilinde işlemler sunar | `ringFlat(NodeId)`, `requestDoorOpen()`, `broadcastHeartbeat()`, `onRing()`, `onDoorOpenRequest()`, `onHeartbeat()` |
| `PowerManager` | platform | CPU frekansı | `begin()` |

**Tasarım kuralları:**
- Constructor'lar sadece ayarları saklar, donanıma dokunmaz. Donanım `begin()` içinde başlatılır, çünkü global nesnelerin constructor'ları Arduino hazır olmadan çalışır.
- Bağımlılıklar constructor'dan referansla verilir (dependency injection). Nesneleri composition root kurar.
- Sanal fonksiyon sadece `Component` hiyerarşisinde, iki uygulaması olan `NodeIdentity` arayüzünde ve Arduino'nun `ESP_NOW_Peer` sınıfında var. Tek uygulaması olan şey için arayüz yazılmıyor (YAGNI).
- Kalıtım yerine composition tercih ediliyor: `Bell`, bir `PulseOutput` **içeriyor**, ondan türemiyor.

### 3.4 Sketch'ler

```cpp
#include "outdoor_unit.h"  // Dış ünite nesneleri

void setup() {
  flatButtons.onPress([](NodeId flat) { intercom.ringFlat(flat); });  // N. daire butonu -> N. dairenin zili
  intercom.onDoorOpenRequest([] { doorOpener.open(); });              // Kapı açma isteği -> kapı açılır
  heartbeatTimer.onTick([] { intercom.broadcastHeartbeat(); });       // Periyot doldu -> "buradayım" yayını
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
```

```cpp
#include "indoor_unit.h"  // İç ünite nesneleri

void setup() {
  openDoorButton.onPress([] { intercom.requestDoorOpen(); });     // Kapıyı aç butonu -> dış üniteye istek
  openDoorButton.onStartupHold([] { identity.startPairing(); });  // Açılışta basılı tutuldu -> eşleştirme modu
  intercom.onRing([] { bell.ring(); });                            // Zil isteği -> zil çalar
  intercom.onHeartbeat([] { linkMonitor.refresh(); });             // Dış üniteden "buradayım" -> bağlantı var
  linkMonitor.onConnected([] { linkLed.turnOn(); });               // Bağlantı sağlandı -> LED sürekli yanar
  linkMonitor.onLost([] { linkLed.blink(); });                     // Bağlantı yok -> LED yanıp söner
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
```

Sketch'ler 1.1'deki tabloyla birebir örtüşüyor. Sketch'lerde mesaj tipi, kimlik ya da süre yok.

### 3.5 Akış örneği: 3. daireye zil

```
Dış ünite                                                     1. daire      2. daire      3. daire
─────────                                                     ────────      ────────      ────────
Buton 3 bırakıldı (50 ms–30 sn, bekleme süresi dolmuş)
→ ButtonGroup → intercom.ringFlat(3)
→ FloodRouter.send(3, kRingBell)
→ SecureChannel.seal: sayaç+1, AES-CCM
→ EspNowRadio: 420 ms burst, 10 ms'de bir ─────────────────→ pencere yakalar
                                                              denetle, doğrula
                                                              hedef ≠ ben → aktar ────→ …
                                                                                        …aktar ─────→ doğrula
                                                                                                      hedef = ben
                                                                                                      sayacı NVS'ye yaz
                                                                                                      intercom.onRing
                                                                                                      bell.ring() 1,5 sn
```

### 3.6 Çerçeve biçimi (v2)

| Alan | Bayt | Koruma |
|---|---|---|
| `version` | 1 | imzalı |
| `apartmentId` | 4 | imzalı |
| `sourceMac` | 6 | imzalı |
| `source` | 1 | imzalı |
| `destination` | 1 | imzalı |
| `counter` | 4 | imzalı |
| `type` | 1 | şifreli + imzalı |
| `tag` | 8 | — |
| **Toplam** | **26** | ESP-NOW sınırı 250 bayt |

- **Nonce (13 bayt):** `sourceMac(6) | counter(4) | version(1) | 0(2)`. Tekilliğini fabrikadan tekil MAC (Karar 4) ve kalıcı sayaç (Karar 13) birlikte garanti ediyor. Apartman kimliği nonce'ta yok, çünkü her apartmanın anahtarı ayrı.
- **Bayt sırası:** Little-endian. Alanlar tek tek yazılır, struct'lar bellekten doğrudan kopyalanmaz.

**Gelen çerçevenin işlenme sırası:** Ucuz denetimler önce, kripto sonra, durum değişikliği en son.
1. Uzunluk 26 bayt mı? (`FloodRouter`)
2. Sürüm ve `apartmentId` bizim mi?
3. `sourceMac` benim mi (kendi yankım) ya da `source`/`destination` olmayan bir ünite mi? Öyleyse at.
4. Gönderen MAC'in tekrar penceresine salt okunur bak. Görülmüş kopyayı şifre çözmeden at (Karar 16).
5. AES-CCM ile doğrula ve şifreyi çöz. Geçmezse at.
6. Gönderen MAC için pencere yoksa şimdi aç ve kayıtlı sayacı NVS'den yükle. Yer sadece doğrulanmış göndericiye ayrılıyor, sahte MAC'ler tabloyu dolduramıyor. Pencereyi ilerlet.
7. Mesaj tipi tanımlı mı?
8. Eşleştirme modundaysam ve bu bir zil isteğiyse hedef daireyi kimliğim olarak kaydet (Karar 4).
9. Hedef herkesse (heartbeat) aktar ve teslim et, kalıcı kayıt yok (Karar 18).
10. Hedef bensem sayacı NVS'ye yaz, yazılamazsa at, sonra eylemi çalıştır. Değilsem çerçeveyi olduğu gibi aktar.

### 3.7 Eşzamanlılık ve zaman

- **Tek iş parçacığı:** Bütün mantık Arduino'nun loop görevinde çalışır.
- **Alma:** ESP-NOW'ın alma geri çağrısı (Wi-Fi görevinde çalışır) sadece çerçeveyi kopyalar, 8 çerçevelik statik bir FreeRTOS kuyruğuna koyar ve loop görevini bildirimle uyandırır. Üretici–tüketici kalıbı: paylaşılan durum yok, kilit yok.
- **Döngü:** `EventLoop` her turda bütün bileşenleri günceller. Sonra en yakın `nextDeadlineMs()` zamanına ya da bir bildirime kadar bloklanır, en fazla 1 sn. İşlemci bu sürede WFI ile bekler.

---

## 4. Veriye dayalı sınırlar

| Parametre | Değer | Dosya | Dayanak |
|---|---|---|---|
| En kısa basış | 50 ms | input_config.h | Sıçrama < 10 ms (Ganssle) · EFT patlaması 15 ms (IEC 61000-4-4) · insan basışı ≈ 80–110 ms |
| Örnekleme periyodu | 5 ms | input_config.h | Ganssle: 1–5 ms |
| En uzun basış | 30 sn | input_config.h | HMI "basılı tut" zaman aşımı pratiği ≥ 30 sn. Tahmin, sahada gözden geçirilebilir. |
| Kapı darbesi | 1,5 sn (1–2 sn) | OutDoor/unit_config.h | Gereksinim |
| Zil darbesi | 1,5 sn (1–2 sn) | InDoor/unit_config.h | Gereksinim |
| Zil bekleme süresi | 3 sn | OutDoor/unit_config.h | 1,5 sn darbe + 1,5 sn sessizlik |
| Kapı isteği bekleme süresi | 2 sn | InDoor/unit_config.h | Mühendislik tercihi |
| Eşleştirme için basılı tutma | 5 sn (≥ 3 sn) | InDoor/unit_config.h | Kazara olmayacak kadar uzun |
| Eşleştirme penceresi | 2 dk (≥ 30 sn) | InDoor/unit_config.h | Dış üniteye yürümeye yetecek süre |
| Heartbeat aralığı | 30 sn (≥ 10 sn) | link_config.h | Her yayın tüm üniteleri 420 ms gönderime sokuyor |
| Bağlantı zaman aşımı | 95 sn (türetilmiş) | link_config.h | 3 × aralık + 5 sn: tek kaçırılan yayın LED'i düşürmez |
| LED yanıp sönme | 0,5 sn yanık / 0,5 sn sönük | InDoor/unit_config.h | Belirgin, göz yormayan hız |
| Uyanma aralığı | 200 ms | radio_config.h | Espressif: "100'ün katları önerilir" (`esp_wifi.h`) · 4 kat en kötü 0,8 sn |
| Uyanma penceresi | 20 ms | radio_config.h | %10 hedefi · burst periyodunun 2 katı |
| Burst periyodu | 10 ms | radio_config.h | Pencere başına ≥ 2 kopya |
| Burst süresi | 420 ms (türetilmiş) | radio_config.h | 2 × aralık + pencere (Karar 10) |
| Aktarma gecikmesi (jitter) | 0–10 ms rastgele | radio_config.h | Aktarıcılar arasında çakışmayı azaltır |
| Tekrar penceresi | 64 | replay_window.h | RFC 6347 / RFC 4303 varsayılanı |
| Sayaç rezervi | 1000 | secure_channel.h | OpenThread `STORE_FRAME_COUNTER_AHEAD` varsayılanı |
| Şifreleme | AES-128-CCM, 8 bayt etiket | ccm_cipher.h | 802.15.4 / Zigbee / Thread / BLE standardı |
| CPU | 80 MHz | power_config.h | Wi-Fi'ın çalıştığı en düşük frekans |
| TX gücü | 8 dBm (başlangıç), Long Range açık | site_config.h | Super Mini anten raporları. Menzil yetmezse artırılır. |
| Watchdog | 5 sn, bekleme ≤ 1 sn | event_loop.h | sdkconfig |

Sınırlar ayar dosyalarında `static_assert` ile denetleniyor. Örnekler: pencere < aralık, burst periyodu ≤ pencere / 2, en kısa basış < en uzun basış, aynı pine iki eleman bağlanamaz, pinler strapping/USB/UART pinine denk gelemez, daire butonu olmayan bir daireye bağlanamaz, apartman kimliği ve anahtarı sıfır olamaz. Yanlış bir değer girildiğinde program derlenmez.

---

## 5. Konvansiyonlar

**Yazım kuralları:**
- Tanımlayıcılar İngilizce, açıklamalar Türkçe ve sadece satır sonunda.
- Sabitler: `kPascalCase`, namespace içinde `inline constexpr`. `#define` kullanılmıyor.
- Tipler PascalCase, fonksiyon ve değişkenler camelCase, private üyeler sonda `_` ile.
- Dosyalar snake_case, başlık koruması `#pragma once`, enum'lar `enum class`.

**Sorumluluk ayrımı:**
- `.ino`: sadece bağlamalar.
- `hardware.h`: sadece dışarıdan bağlanan elemanlar.
- Ayarlar header dosyalarında. Siteye özgü değerler `site_config.h`, ürün ayarları `*_config.h` içinde.
- Protokolün değişmez sabitleri (çerçeve boyutu, pencere 64) kodda durur, ayar dosyasında değil. Bunları değiştirmek uyumluluğu bozar.

**Bellek ve dil özellikleri:**
- `setup()`'tan sonra heap kullanılmaz. `String`, `std::function` ve `new` yok.
- Lambdalar değişken yakalamaz, düz fonksiyon işaretçisine dönüşür.
- Exception, RTTI ve log kullanılmaz. Hatalar dönüş değeriyle bildirilir (Karar 17).
- Donanıma sahip olan sınıflar kopyalanamaz. Tek parametreli constructor'lar `explicit`.

**Kod yerleşimi:**
- Ortak kodun tamamı `.h` dosyalarında (`inline`), çünkü Arduino IDE sketch klasörü dışındaki `.cpp` dosyalarını derlemiyor.
- `.ino` içinde sadece bağlamalar olur. Arduino'nun `.ino` ön işlemcisi modern C++'ı bozabiliyor (ör. `consteval`), bu kod `.h` içinde durur.
- Core katmanındaki dosyalar `Arduino.h` include etmez.

### 5.1 C++ araçları

Derleyici GCC 14.2, C++20 modunda. Her araç sadece gerçekten işe yaradığı yerde kullanılır.

| Araç | Nerede | Neden |
|---|---|---|
| Soyut sınıf | `Component` | Birden çok sınıf uyguluyor, `EventLoop` hepsini tek tip görüyor |
| Sınıf template'i + CTAD | `ButtonGroup<N>` | N, pin tablosunun boyutundan otomatik çıkarılır |
| Fonksiyon template'i (`consteval`) | `allUnique(items, key)` | Aynı denetim iki alanda: buton pinleri, daire numaraları (DRY) |
| Fonksiyon template'i (concept kısıtlı) | `writeLe<T>` / `readLe<T>` | Her tamsayı genişliği için tek kod, işaretsiz olmayan tipler derlenmez |
| `std::bit_cast`, `std::endian` | `byte_order.h` | Bayt dönüşümü standart kütüphaneyle, little-endian varsayımı derleme anında denetlenir |
| `std::array` | Tekrar pencereleri, MAC adresi, çerçeve | Sabit boyut, heap yok |
| `std::optional` | `SecureChannel::seal/open()`, `loadRxCounter()` | "Sonuç yok" durumunu tipin kendisi ifade eder |
| `std::span` | Bayt tamponları, çerçeve alanları | İşaretçi + uzunluk çiftinin güvenli karşılığı |
| `<algorithm>` | Pin denetimi, anahtar denetimi, bayt kopyalama | Elle döngü yerine standart algoritma |
| `[[nodiscard]]` | `open()`, `seal()`, `begin()`, `isFresh()`, `receive()`, `save…()` | Doğrulama ve kayıt sonucu kontrol edilmeden bırakılırsa derleyici uyarır |
| `static_assert` | Ayar ve kablolama dosyaları | Ayarlar için derleme zamanı sözleşmesi |

**Template yazma ölçütü:** Aynı kod farklı tipler ya da derleme zamanında bilinen farklı boyutlar için gerekiyorsa template yazılır. Tek tip varsa normal sınıf yazılır.

**Kullanılmayanlar:**
- CRTP: `EventLoop` farklı tipteki bileşenleri tek listede tutuyor. Bu çalışma anı polimorfizmi gerektiriyor.
- Policy-based design, variadic template zincirleri, template metaprogramming: Her politikanın tek uygulaması var. Okunabilirliği ağırlaştırırlar, karşılığında bir şey kazandırmazlar.
- `std::vector`, `std::string`, `std::function`, `std::map`: Heap kullanıyorlar.
- Çalışma anında eklenebilir middleware zinciri: Adımlar çalışma anında değişmiyor (Karar 6). Her yere dokunan bir ihtiyaç doğarsa Decorator kullanılır.

### 5.2 Kütüphane kullanımı: Arduino-first

Sıra: önce Arduino-ESP32 core'un API ve kütüphaneleri, Arduino karşılığı yoksa ESP-IDF/FreeRTOS, hazır bir şey yoksa elle yazılan kod.

**Arduino:**

| İş | API |
|---|---|
| Wi-Fi modu, Long Range, modem uykusu, kanal, TX gücü, MAC adresi | `WiFi` (`enableLongRange`, `mode`, `setSleep`, `setChannel`, `setTxPower`, `macAddress`) |
| ESP-NOW başlatma, yayın eşi ve Long Range hızı, alma | `ESP_NOW` (`ESP_NOW.begin`, `ESP_NOW_Peer`, `onNewPeer`) |
| Kalıcı sayaçlar | `Preferences` (NVS) |
| Rastgele aktarma gecikmesi | `random()` (Wi-Fi açıkken donanım RNG) |
| Pin okuma/yazma | `pinMode`, `digitalRead`, `digitalWrite` |
| CPU frekansı | `setCpuFrequencyMhz()` |
| Watchdog | `enableLoopWDT()` |

**Arduino karşılığı olmadığı için ESP-IDF / FreeRTOS:**

| İş | API | Neden |
|---|---|---|
| ESP-NOW uyanma penceresi ve aralığı | `esp_now_set_wake_window`, `esp_wifi_connectionless_module_set_wake_interval` | Arduino `WiFi` ve `ESP_NOW` bu ayarları sunmuyor |
| 64-bit zaman | `esp_timer_get_time` | Arduino `millis()` 32-bit, 49,7 günde taşar (Karar 8) |
| Çıkışı açmadan önce pasif seviye yazmak | `gpio_set_level` | Arduino 3.x'te `digitalWrite()` `pinMode()`'dan önce çalışmıyor, açılışta röle titreyebilirdi |
| AES-128-CCM | mbedTLS `mbedtls_ccm_*` | Arduino core'da AES-CCM sarmalayıcısı yok. mbedTLS core'la birlikte geliyor, C3'te donanım AES kullanıyor. |
| Olay bekleme, alma kuyruğu | FreeRTOS task notification, statik kuyruk | Arduino'da görevler arası kuyruk ve bildirim API'si yok |

**Elle yazılanlar:**

| Kod | Neden hazır değil |
|---|---|
| `PressDetector` | Bounce2, OneButton gibi kütüphaneler en kısa/en uzun basış, bekleme süresi ve açılışta basılı butonu yok sayma kurallarını birlikte sağlamıyor. Kural katmanı yine yazılacağı için ek bağımlılık bir şey kazandırmıyor. |
| `ReplayWindow` | Arduino ve ESP-IDF'te bağımsız kullanılabilir bir tekrar penceresi yok. mbedTLS'inki DTLS oturumunun içinde. |
| Burst ve flooding (`EspNowRadio`, `FloodRouter`) | Arduino core'da flooding yapan bir kütüphane yok. Espressif'in aktarma destekli `esp-now` bileşeni ESP-IDF'e geçiş gerektirir. |
| `EventLoop` / `Component` | "En yakın zamana ya da ESP-NOW bildirimine kadar bekle" davranışını FreeRTOS sağlıyor. Üstündeki katman ince bir liste. |
| `PulseOutput` | Arduino `Ticker` geri çağrıyı başka bir görevde çalıştırıyor. Paylaşılan durum ve kilit gerekirdi. Zamanlamayı olay döngüsü zaten yapıyor. |
| `frame`, `protocol` | Uygulamaya özgü çerçeve biçimi |
| Bayt sırası, arama, denetimler | C++ standart kütüphanesiyle (`std::bit_cast`, `std::find`, `<algorithm>`) |

---

## 6. Durum

**Kod:** Tüm işlevler yazıldı: G/Ç, ağ ve aktarma, güvenlik, güç tasarrufu, denetimler.

**Kurulum için gerekenler:**
1. `Common/config/site_config.example.h` → `site_config.h` olarak kopyalanır. Apartman kimliği ve anahtar rastgele doldurulur.
2. `OutDoor` dış üniteye, aynı `InDoor` dört iç üniteye yüklenir. Ünite başına ayar yok.
3. Her iç ünite eşleştirilir: ünite fişe takılır (eşleşmemişse kendiliğinden eşleştirme modundadır), dış ünitede o dairenin butonuna basılır, zil çalınca eşleşme tamamdır (Karar 4).
4. Kart: ESP32C3 Dev Module. "Erase All Flash Before Sketch Upload" kapalı (Karar 13). Kod seri port kullanmıyor.

## 7. Senin kararını bekleyenler (donanım)

1. Dış ünite ESP'si bina içine alınacak mı? (Karar 2)
2. Kanarya zil 220V ile mi çalışıyor? Öyleyse izolasyonlu, 3.3V ile tetiklenebilen (high-level trigger) bir röle modülü gerekir.
3. Kapı rölesinin girişi nasıl: ortak GND mi, optokuplör mü? Ne kadar akım çekiyor?
4. Dışarıdaki uzun buton hatları için RC filtre önerisi: pin ile buton arasına 1 kΩ seri direnç, pin ile GND arasına 100 nF kondansatör.
5. Adaptör: kaliteli ve en az 1 A. Yüksek TX gücünde anlık akım yüzlerce mA'e çıkabiliyor, ucuz adaptörler brownout'a yol açar.

---

## Kaynaklar

- ESP32-C3 datasheet (güç tüketimi): https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf
- Kurulu core yapılandırması: `%LOCALAPPDATA%/Arduino15/packages/esp32/tools/esp32c3-libs/3.3.11/sdkconfig`
- `esp_wifi.h` / `esp_now.h` (uyanma aralığı ve penceresi notları): aynı klasörde `include/esp_wifi/include/`
- Ganssle, A Guide to Debouncing: https://www.ganssle.com/debouncing.htm · https://www.ganssle.com/debouncing-pt2.htm
- IEC 61000-4-4 EFT/Burst: https://en.wikipedia.org/wiki/IEC_61000-4-4
- Tuş basılı tutma süreleri: https://wraitor.io/learn/keystroke-dynamics
- RFC 6347 (DTLS 1.2) tekrar penceresi: https://www.rfc-editor.org/rfc/rfc6347
- OpenThread `STORE_FRAME_COUNTER_AHEAD`: https://openthread.io/reference/config/group/config-misc
- HMI buton zaman aşımı pratiği: https://industrialmonitordirect.com/blogs/knowledgebase/hmi-button-set-while-pressed-timeout-issues-and-best-practices
