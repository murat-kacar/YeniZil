# YeniZil — Mimari ve Uygulama Planı

Sürüm 1 · 28.09.2026 · Durum: **onay bekliyor**

Bu belge şimdiye kadar yapılan her şeyin gözden geçirilmesiyle sıfırdan hazırlandı. Mevcut kod referans alındı ama bağlayıcı değil; neyin değiştiği ve neden değiştiği 2. bölümde.

---

## 1. Kapsam

### 1.1 Gereksinimler

- 4 katlı bina, her katta 1 daire.
- **Dış ünite:** 4 zil butonu + kapı rölesi tetiği (3.3V).
- **İç ünite (×4):** "kapıyı aç" butonu + zil tetiği (3.3V).
- Her ünite kendi 5V adaptöründen beslenir. Pil yok.
- Tamamen kablosuz, internet yok. ESP-NOW ile flooding.
- Derleme ortamı: Arduino IDE, arduino-cli 1.5.1, esp32 core 3.3.11 (ESP-IDF 5.5.5).

| Tetikleyici | Sonuç |
|---|---|
| Dış ünitede N. butona basılıp bırakıldı | N. dairenin zili 1,5 sn çalar |
| N. dairede "kapıyı aç" butonuna basılıp bırakıldı | Kapı rölesi 1,5 sn tetiklenir |

### 1.2 Kapsam dışı (YAGNI)

İhtiyaç doğduğunda eklenecek, şimdi tasarıma girmiyor:

- Geri bildirim LED'leri
- "Kapı sadece zil çaldıktan sonra açılabilsin" kuralı
- Zaman senkronizasyonu (FTSP)
- OTA güncelleme ve flash şifreleme
- ESP-NOW Long Range modu (menzil testi gerektirirse)
- Otomatik hafif uyku (ESP-IDF'e geçiş gerektirir, bkz. Bulgu 1)

---

## 2. Gözden geçirme: bulgular ve kararlar

### Bulgu 1 — Otomatik hafif uyku bu core'da yok

Kurulu core'un `sdkconfig` dosyasında:
- `CONFIG_PM_ENABLE` kapalı. Yani işlemcinin boşta otomatik uykuya geçmesi mümkün değil.
- `CONFIG_ESP_WIFI_STA_DISCONNECTED_PM_ENABLE=y`. Yani ESP-NOW uyanma penceresi (%10) çalışıyor.

Beklenen tüketim (C3 datasheet: alım 84 mA, 80 MHz'de modem-sleep ve işlemci boşta 13–18 mA):

| Mod | Ortalama akım | Güç (≈) |
|---|---|---|
| Sürekli dinleme | ≈ 85–100 mA | ≈ 0,45 W |
| %10 pencere | 0,1 × 84 + 0,9 × 13–18 ≈ **22–25 mA** | ≈ **0,12 W** |

**Karar:** Radyo %10 ile çalışacak, işlemci boşta WFI komutuyla bekleyecek. Tüketim ~3,5 kat düşüyor, ısınma ihmal edilebilir hale geliyor. `autoLightSleep` ayarı, WakeLock, `gpio_hold` ve GPIO ile uyandırma gereksiz; plandan çıkarıldı. Daha fazla tasarruf gerekirse ESP-IDF'e geçmek ayrı bir karar olur.

### Bulgu 2 — Dış ünite fiziksel olarak savunmasız

Anahtar ve röle tetik kablosu bina dışındaki kutuda duruyor. Kutuyu açan biri tetik kablosunu 3.3V'a değdirip kapıyı açabilir. Ya da flash'ı okuyup anahtarı alabilir.

**Karar:** ESP ve röle tetiği bina içinde, kapının güvenli tarafında olacak. Dışarıda sadece butonlar kalacak. Bu, erişim kontrol sistemlerinde yaygın bir kurulum kuralı: denetleyici güvenli tarafta, okuyucu dışarıda. Aynı karar yağmur, güneş ve ısınma sorununu da çözüyor.

### Bulgu 3 — Yeniden başlatmaya dayalı tekrar saldırısı

Alıcının tekrar koruma durumu RAM'de tutuluyordu. Saldırı senaryosu:
1. Saldırgan bir "kapıyı aç" mesajını havadan kaydeder.
2. Dış ünitenin fişini çekip takar. Tekrar koruma durumu sıfırlanır.
3. Kaydettiği mesajı tekrar gönderir, kapı açılır.

**Karar:** Kendine gelen ve eyleme dönüşecek her mesajın sayacı, **eylemden önce** NVS'ye yazılacak. Açılışta tekrar penceresi "kayıtlı sayaca kadar hepsi görüldü" durumuyla başlayacak.

Flash ömrü açısından sorun yok. NVS aşınmayı dengeliyor. Günde 50 olay × 5 ünite, 100 bin silme döngülük flash ömrünün çok altında kalıyor.

### Bulgu 4 — Kimlik çakışması şifrelemeyi kırar

İki ünite yanlışlıkla aynı `FLAT_ID` ile yüklenirse aynı (kaynak, sayaç) çiftleri üretilir. Bu da aynı nonce demek ve AES-CCM'in güvenliği tamamen çöker.

**Karar:** Kimlik elle yazılmayacak. Ortak `kNodeMacs[]` tablosunda her kartın MAC adresi yer alacak, kimlik tablodaki sıra olacak: 0 = dış ünite, 1–4 = daireler. Tablonun tekrarsız olduğu `static_assert` ile derleme anında denetlenecek. Ek bir faydası da var: dört iç ünite aynı firmware'i kullanabilir, tek derleme dört kez yüklenir.

### Bulgu 5 — TTL gereksiz

Tekrar koruması her kaynak için tam çalışıyor (kayan pencere). Bu yüzden bir düğüm aynı mesajı en fazla bir kez aktarıyor ve flooding'in sonlanması zaten garanti.

**Karar:** `hopLimit` alanı çıkarıldı. Böylece başlığın tamamı imzalı hale geldi.

### Bulgu 6 — Middleware zinciri yerine sabit sıralı işlem hattı

Güvenlik adımlarının sırası kritik. Önce doğrulama, sonra tekrar denetimi yapılmalı. Ters sıra, sahte yüksek sayaçlarla gerçek mesajların engellenmesine (DoS) yol açar. Çalışma anında yeniden sıralanabilen bir zincir bu hatayı mümkün kılar. Ayrıca aşamaların giriş ve çıkış tipleri farklı, ortak bir arayüze zorlamak için her şeyi taşıyan bir "bağlam" nesnesi gerekirdi.

**Karar:** Adımlar `SecureChannel` içinde sabit sırayla çalışacak. Bu, Pipes and Filters kalıbının statik biçimi. Senin önerdiğin middleware yaklaşımının amacı korunuyor: her adım ayrı bir sınıf. Tek fark, sıranın değiştirilememesi.

### Bulgu 7 — `Rule<T>` ve `RuleSet<T>` şimdilik gereksiz

Buton denetimlerinin hepsi bir basışın sayısal sınırları: en kısa süre, en uzun süre, bekleme süresi. Bunlar birer ayar değeri, ayrı kural sınıfları değil.

**Karar:** Sınırlar `PressConfig` yapısında duracak. Farklı türde bir denetim ortaya çıktığında soyutlama eklenecek (Rule of Three).

### Bulgu 8 — Aynı ihtiyaç iki yerde

Dış ünitedeki daire başına zil bekleme süresi ile iç ünitedeki kapı isteği bekleme süresi aynı ihtiyaç: buton başına bekleme.

**Karar:** Tek bir mekanizma olacak: `PressConfig.cooldownMs` (DRY).

### Bulgu 9 — `millis()` 49,7 günde taşar

**Karar:** Zaman 64-bit, `esp_timer` tabanlı `nowMs()` fonksiyonundan alınacak. Taşma hatası sınıfı tamamen ortadan kalkıyor.

### Bulgu 10 — Butonlar kesmeyle değil, örneklemeyle okunacak

**Karar:** Butonlar 5 ms'de bir okunacak. Ganssle'ın önerisi 1–5 ms'lik örnekleme. İşlemci zaten uykuya geçmediği için (Bulgu 1) bunun ek maliyeti yok. Kesme kullanmaktan ve uyku seviyesini değiştirmekten kaynaklanan karmaşıklık da ortadan kalkıyor.

### Bulgu 11 — Güvenilirlik hesabı: burst uzamalı

220 ms'lik bir burst alıcıya 1 pencere ve o pencere içinde yaklaşık 2 kopya veriyor.

Tek bir çerçevenin ulaşma oranı %90 olan bir kat geçişinde:
- Mesajın kaçırılma olasılığı 0,1² = %1
- 4 katta toplam ≈ %4. Bu kabul edilemez.

**Karar:** Burst süresi = 2 × aralık + pencere = **420 ms**. Böylece alıcı 2 pencere, her pencerede 2 kopya görüyor:
- Kaçırma olasılığı 0,1⁴ = kat başına %0,01
- 4 katta ≈ %0,04

Gecikme değişmiyor, çünkü mesaj yine ilk pencerede yakalanıyor.

### Bulgu 12 — Sırlar kaynak koddaydı

**Karar:** Siteye özgü değerler (apartman kimliği, anahtar, MAC tablosu, kanal, TX gücü) `site_config.h` dosyasına taşınacak. Bu dosya git dışında kalacak. Yanında bir `site_config.example.h` şablonu ve anahtar üreten bir betik olacak. Arduino'nun kendi `arduino_secrets.h` alışkanlığıyla aynı fikir.

### Bulgu 13 — Şifreleme algoritması

**Karar:** AES-128-CCM, 8 bayt doğrulama etiketi. IEEE 802.15.4, Zigbee, Thread ve BLE'nin kullandığı standart. C3'te donanım hızlandırması var: `CONFIG_MBEDTLS_HARDWARE_AES=y`, `CONFIG_MBEDTLS_CCM_C=y`. Etiket 4 bayt yerine 8 bayt, çünkü kapı açma kritik bir işlem.

### Bulgu 14 — Radyo ayarları radyonun işi

TX gücü, pencere ve aralık radyonun kendi ayarları.

**Karar:** Bu ayarlar `EspNowRadio` sınıfına (`RadioConfig`) taşındı. `PowerManager` sadece CPU frekansını yönetecek. Böylece "radyo önce başlamalı" gibi bir sıra bağımlılığı da kalmıyor.

### Bulgu 15 — Watchdog

`CONFIG_ESP_TASK_WDT_TIMEOUT_S=5`.

**Karar:** Döngü görevi watchdog'a bağlanacak ve olay beklemesi en fazla 1 sn sürecek. Kod kilitlenirse cihaz yeniden başlar. Röle pini pull-down'da olduğu için yeniden başlama kapıyı açmaz.

### Bulgu 16 — Tekrarlanabilir derleme

Arduino IDE'nin kart menüsündeki ayarlar kaynak kontrolünde tutulmuyor.

**Karar:** Her sketch klasörüne bir `sketch.yaml` profili eklenecek. Core sürümü (3.3.11) ve FQBN seçenekleri burada sabitlenecek.

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
│ Platform   EspNowRadio · CounterStore · PowerManager      │
│            (ESP-IDF / Arduino'ya dokunan tek katman)       │
├─────────────────────────────────────────────────────────┤
│ Core       PressDetector · ReplayWindow · frame · protocol │
│            (saf C++: Arduino/IDF include etmez)            │
├─────────────────────────────────────────────────────────┤
│ Kernel     EventLoop · Component · nowMs()                │
└─────────────────────────────────────────────────────────┘
```

Kurallar:
- Bağımlılık sadece aşağı doğru olabilir.
- Config her katmana değer verir ama hiçbir katmana bağımlı değildir.
- Core katmanındaki dosyalar platforma dokunmaz. Bu yüzden bilgisayarda test edilebilirler.

### 3.2 Klasör yapısı (özelliğe göre paketleme)

```
YeniZil/
├── OutDoor/
│   ├── OutDoor.ino           bağlamalar
│   ├── outdoor_unit.h        composition root
│   ├── hardware.h            kablolama: buton ve röle pinleri
│   ├── unit_config.h         dış üniteye özel ayarlar
│   └── sketch.yaml           derleme profili
├── InDoor/
│   ├── InDoor.ino
│   ├── indoor_unit.h
│   ├── hardware.h
│   ├── unit_config.h
│   └── sketch.yaml
├── Common/
│   ├── config/
│   │   ├── site_config.example.h   şablon (git'te)
│   │   ├── site_config.h           gerçek değerler (git dışı)
│   │   ├── radio_config.h          pencere, aralık, burst
│   │   ├── input_config.h          basış sınırları
│   │   └── power_config.h          CPU frekansı
│   ├── kernel/   clock.h · component.h · event_loop.h
│   ├── io/       press_detector.h (saf) · button.h · button_group.h · pulse_output.h
│   ├── net/      protocol.h (saf) · frame.h (saf) · node_identity.h · esp_now_radio.h · flood_router.h
│   ├── security/ replay_window.h (saf) · ccm_cipher.h · counter_store.h · secure_channel.h
│   ├── power/    power_manager.h
│   └── app/      intercom.h · bell.h · door_opener.h
├── Tools/
│   ├── RangeTest/RangeTest.ino     menzil + RSSI + MAC yazdırma
│   └── new_site_config.ps1         rastgele apartman kimliği ve anahtar üretir
├── docs/ARCHITECTURE.md
└── .gitignore                      site_config.h
```

### 3.3 Sınıflar

| Sınıf | Tür | Sorumluluk | Public arayüz |
|---|---|---|---|
| `monotonicMs()` | platform | 64-bit monoton zaman (ms) | `uint64_t monotonicMs()` |
| `Component` | soyut | Güncellenen her şeyin ortak arayüzü. Constructor'da kendini zincire ekler (intrusive list, heap yok). | `begin()`, `update(nowMs)`, `nextDeadlineMs()` |
| `PollingComponent` | soyut | Sabit periyotla örnekleme (Template Method). `Button` ve `ButtonGroup` ortak zamanlamayı buradan alır. | `poll(nowMs)` (korumalı) |
| `board::isSafeGpio()` | saf | Super Mini'de açılışı etkilemeyen pinler. `hardware.h` bunu `static_assert` ile kullanır. | `constexpr bool isSafeGpio(pin)` |
| `EventLoop` | kernel | Bileşenleri başlatır ve günceller. En yakın zamana ya da bir bildirime kadar bloklanır (≤ 1 sn). Watchdog'u besler. | `begin()`, `update()`, `notify()` |
| `PressDetector` | saf | Seviye ve zamandan geçerli basışı çıkarır: süre sınırları, bekleme süresi, açılışta basılı gelen butonu bırakılana kadar yok sayma. | `bool update(bool pressed, uint64_t nowMs)` |
| `Button` | servis | Tek buton: 5 ms örnekleme + `PressDetector` | `onPress(void(*)())` |
| `ButtonGroup<N>` | servis | N buton. Her butonun bir kimliği var, olay kimlikle gelir. | `onPress(void(*)(NodeId))` |
| `PulseOutput` | servis | Belirli süre aktif kalan çıkış. Aktifken gelen tetik yok sayılır. Açılışta hemen pasife çekilir. | `activate()` |
| `Bell` / `DoorOpener` | app | Alan dilinde eylem (composition ile `PulseOutput` kullanır) | `ring()` / `open()` |
| `protocol` | saf | `NodeId`, `MessageType`, sürüm, sabitler | — |
| `frame` | saf | Çerçeveyi bayt bayt yazar ve okur (struct kopyalama yok) | `encodeHeader()`, `decodeHeader()` |
| `NodeIdentity` | platform | Kendi MAC'ini tabloda bulur, rolünü doğrular | `NodeId self()` |
| `EspNowRadio` | platform | ESP-NOW başlatma, kanal, TX gücü, uyanma penceresi, burst gönderim, alma kuyruğu | `broadcast(const Frame&)`, `bool receive(Frame&)` |
| `ReplayWindow` | saf | 64'lük kayan pencere | `isFresh()`, `markSeen()`, `restore()` |
| `CcmCipher` | taşınabilir (mbedTLS) | AES-128-CCM ile şifreleme ve doğrulama | `seal()`, `bool open()` |
| `CounterStore` | platform | NVS: gönderme sayacı rezervi, kaynak başına en yüksek sayaç | `load…()`, `save…()` |
| `SecureChannel` | servis | Sabit sıra: doğrula → tekrar denetimi → (kendine geldiyse) kalıcı kaydet | `Frame seal(dst, type)`, `[[nodiscard]] std::optional<Message> open(frame)` |
| `FloodRouter` | servis | Gelen çerçeveyi süzer, kendine geleni teslim eder, gerisini **değiştirmeden** aktarır | `send(dst, type)`, `on(type, void(*)())` |
| `Intercom` | app (facade) | Protokolü gizler, alan dilinde işlemler sunar | `ringFlat(NodeId)`, `requestDoorOpen()`, `onRing()`, `onDoorOpenRequest()` |
| `PowerManager` | platform | CPU frekansı | `begin()` |

**Tasarım kuralları:**
- Constructor'lar sadece ayarları saklar, donanıma dokunmaz. Donanım `begin()` içinde başlatılır, çünkü global nesnelerin constructor'ları Arduino hazır olmadan çalışır.
- Bağımlılıklar constructor'dan referansla verilir (dependency injection). Nesneleri composition root kurar.
- Sanal fonksiyon sadece `Component` arayüzünde var. Tek uygulaması olan şeyler için arayüz yazılmıyor (YAGNI).
- Kalıtım yerine composition tercih ediliyor: `Bell`, bir `PulseOutput` **içeriyor**, ondan türemiyor.

### 3.4 Sketch'lerin son hali

```cpp
#include "outdoor_unit.h"  // Dış ünite nesneleri

void setup() {
  flatButtons.onPress([](NodeId flat) { intercom.ringFlat(flat); });  // N. daire butonu -> N. dairenin zili
  intercom.onDoorOpenRequest([] { doorOpener.open(); });              // Kapı açma isteği -> kapı açılır
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
```

```cpp
#include "indoor_unit.h"  // İç ünite nesneleri

void setup() {
  openDoorButton.onPress([] { intercom.requestDoorOpen(); });  // Kapıyı aç butonu -> dış üniteye istek
  intercom.onRing([] { bell.ring(); });                         // Zil isteği -> zil çalar
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
                                                              doğrula, tekrar denetimi
                                                              hedef ≠ ben → aktar ────→ …
                                                                                        …aktar ─────→ doğrula
                                                                                                      hedef = ben
                                                                                                      sayacı NVS'ye yaz
                                                                                                      intercom.onRing
                                                                                                      bell.ring() 1,5 sn
```

### 3.6 Çerçeve biçimi (v1)

| Alan | Bayt | Koruma |
|---|---|---|
| `version` | 1 | imzalı |
| `apartmentId` | 4 | imzalı |
| `source` | 1 | imzalı |
| `destination` | 1 | imzalı |
| `counter` | 4 | imzalı |
| `type` | 1 | şifreli + imzalı |
| `tag` | 8 | — |
| **Toplam** | **20** | ESP-NOW sınırı 250 bayt |

- **Nonce (13 bayt):** `apartmentId(4) | source(1) | counter(4) | version(1) | 0(3)`. Tekilliğini kalıcı sayaç (Bulgu 3) ve tekrarsız kimlik tablosu (Bulgu 4) birlikte garanti ediyor.
- **Bayt sırası:** Little-endian. Alanlar tek tek yazılır, struct'lar bellekten doğrudan kopyalanmaz (padding ve endian sorunu olmasın diye).

**Gelen çerçevenin işlenme sırası:** Ucuz denetimler önce, kripto sonra, durum değişikliği en son.
1. Uzunluk ve sürüm doğru mu?
2. `apartmentId` bizim apartmanın mı?
3. `source` ben miyim? Öyleyse kendi yankım, at.
4. AES-CCM ile doğrula ve şifreyi çöz.
5. Tekrar penceresine bak.
6. Hedef ben miysem: sayacı NVS'ye yaz, sonra eylemi çalıştır. Değilsem: çerçeveyi olduğu gibi aktar.

### 3.7 Eşzamanlılık ve zaman

- **Tek iş parçacığı:** Bütün mantık Arduino'nun loop görevinde çalışır.
- **Alma:** ESP-NOW'ın alma geri çağrısı (Wi-Fi görevinde çalışır) sadece çerçeveyi kopyalar, 8 çerçevelik bir FreeRTOS kuyruğuna koyar ve loop görevini bildirimle uyandırır. Klasik üretici–tüketici kalıbı: paylaşılan durum yok, kilit yok.
- **Döngü:** `EventLoop` her turda bütün bileşenleri günceller. Sonra en yakın `nextDeadlineMs()` zamanına kadar ya da bir bildirim gelene kadar bloklanır, en fazla 1 sn. İşlemci bu sürede WFI ile bekler.

---

## 4. Veriye dayalı sınırlar

| Parametre | Değer | Dosya | Dayanak |
|---|---|---|---|
| En kısa basış | 50 ms | input_config.h | Sıçrama < 10 ms (Ganssle) · EFT patlaması 15 ms (IEC 61000-4-4) · insan basışı ≈ 80–110 ms |
| Örnekleme periyodu | 5 ms | input_config.h | Ganssle: 1–5 ms |
| En uzun basış | 30 sn | input_config.h | HMI "basılı tut" zaman aşımı pratiği ≥ 30 sn. **Tahmin, sahada gözden geçirilecek.** |
| Kapı darbesi | 1,5 sn (`static_assert` 1–2 sn) | OutDoor/unit_config.h | Senin gereksinimin |
| Zil darbesi | 1,5 sn (`static_assert` 1–2 sn) | InDoor/unit_config.h | Senin gereksinimin |
| Zil bekleme süresi | 3 sn | OutDoor/unit_config.h | Mühendislik tercihi: 1,5 sn darbe + 1,5 sn sessizlik |
| Kapı isteği bekleme süresi | 2 sn | InDoor/unit_config.h | Mühendislik tercihi |
| Uyanma aralığı | 200 ms | radio_config.h | Espressif: "100'ün katları önerilir" (`esp_wifi.h`) · 4 kat en kötü 0,8 sn |
| Uyanma penceresi | 20 ms | radio_config.h | %10 hedefi · burst periyodunun 2 katı |
| Burst periyodu | 10 ms | radio_config.h | Pencere başına ≥ 2 kopya |
| Burst süresi | 420 ms (türetilmiş) | radio_config.h | 2 × aralık + pencere (Bulgu 11) |
| Aktarma gecikmesi (jitter) | 0–10 ms rastgele | radio_config.h | Aktarıcılar arasında çakışmayı azaltır |
| Tekrar penceresi | 64 | replay_window.h | RFC 6347 / RFC 4303 varsayılanı |
| Sayaç rezervi | 1000 | secure_channel.h | OpenThread `STORE_FRAME_COUNTER_AHEAD` varsayılanı |
| Şifreleme | AES-128-CCM, 8 bayt etiket | ccm_cipher.h | 802.15.4 / Zigbee / Thread / BLE standardı |
| CPU | 80 MHz | power_config.h | Wi-Fi'ın çalıştığı en düşük frekans |
| TX gücü | 8 dBm (başlangıç) | site_config.h | Super Mini anten raporları. **Menzil testiyle belirlenecek.** |
| Watchdog | 5 sn, bekleme ≤ 1 sn | event_loop.h | sdkconfig |

Sınırlar ayar dosyalarında `static_assert` ile denetleniyor. Örneğin `pencere < aralık`, `burst periyodu ≤ pencere / 2`, `en kısa basış < en uzun basış`, "aynı pine iki eleman bağlanamaz", "MAC tablosunda tekrar olamaz". Yanlış bir değer girildiğinde program derlenmez.

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
- Exception ve RTTI kullanılmaz. Hatalar dönüş değeriyle bildirilir, `log_e/w/i` ile loglanır. Log seviyesi Arduino'nun "Core Debug Level" menüsünden seçilir.
- Donanıma sahip olan sınıflar kopyalanamaz. Constructor'lar `explicit`.

**Kod yerleşimi:**
- Ortak kodun tamamı `.h` dosyalarında (`inline`). Arduino IDE sketch klasörü dışındaki `.cpp` dosyalarını derlemiyor, bu test edildi.
- `.ino` içinde sadece bağlamalar olur. Arduino'nun `.ino` ön işlemcisi modern C++'ı bozabiliyor: `consteval` içeren bir fonksiyon `.ino`'da derlenmedi, aynı kod `.h` içinde derlendi (test edildi).
- Core katmanındaki dosyalar `Arduino.h` include etmez.

### 5.1 C++ araçları

Derleyici GCC 14.2, C++20 modunda (`-std=gnu++2a`). Aşağıdakiler bu ortamda derlenerek doğrulandı. Her araç sadece gerçekten işe yaradığı yerde kullanılır.

| Araç | Nerede | Neden |
|---|---|---|
| Soyut sınıf | `Component` | 6 farklı sınıf uyguluyor. `EventLoop` hepsini tek tip görüyor. |
| Arayüz (test sınırı) | `Radio`, `CounterStore` | Sadece bilgisayarda test seçilirse. Sahte (fake) uygulama ikinci uygulama olur. Kural: ikinci uygulama yoksa arayüz de yok. |
| Kendi sınıf template'imiz + CTAD | `ButtonGroup<N>` | N, pin tablosunun boyutundan otomatik çıkarılır |
| Kendi fonksiyon template'imiz (`consteval`) | `allUnique(items, key)` | Aynı denetim üç tabloda: MAC'ler, buton pinleri, daire numaraları (DRY). Aynı pine iki buton yazılırsa derleme durur (test edildi). |
| Kendi fonksiyon template'imiz (concept kısıtlı) | `writeLe<T>` / `readLe<T>` | Tamsayıyı little-endian yazar/okur. Her tamsayı genişliği için tek kod, `std::unsigned_integral` dışındaki tipler derlenmez. |
| `std::array` | Tekrar pencereleri, MAC tablosu | Sabit boyut, heap yok |
| `std::optional` | `SecureChannel::open()`, `frame::decodeHeader()` | "Sonuç yok" durumunu `bool` + çıkış parametresi yerine tipin kendisi ifade eder |
| `std::span` | Bayt tamponları | İşaretçi + uzunluk çiftinin güvenli karşılığı |
| `<algorithm>` | MAC tablosunda arama, tekrar kontrolü | Elle döngü yerine standart algoritma |
| `[[nodiscard]]` | `open()`, `isFresh()`, `receive()` | Doğrulama sonucu kontrol edilmeden bırakılırsa derleyici uyarır. Test edildi. |
| `consteval` + `static_assert` | Ayar dosyaları | Ayarlar için derleme zamanı sözleşmesi, ör. "MAC tablosu tekrarsız" |

**Template yazma ölçütü:** Aynı kod farklı tipler ya da derleme zamanında bilinen farklı boyutlar için gerekiyorsa template yazılır. Tek tip varsa normal sınıf yazılır.

**Kullanılmayanlar:**
- CRTP (`Component` yerine): `EventLoop` farklı tipteki bileşenleri tek bir listede tutuyor. Bu, çalışma anı polimorfizmi (sanal fonksiyon) gerektiriyor. CRTP bunu tek başına sağlamıyor.
- Policy-based design, variadic template zincirleri, template metaprogramming: Her politikanın tek uygulaması var. Okunabilirliği ve derleyici hata mesajlarını ağırlaştırırlar, karşılığında bir şey kazandırmazlar.
- `std::vector`, `std::string`, `std::function`, `std::map`: Heap kullanıyorlar.
- Çalışma anında davranış ekleyen attribute'lar: C++'ta yok. C++26'ya statik yansıma (reflection) girdi, ama derleyicimiz C++20 modunda.
- Çalışma anında eklenebilir middleware zinciri: Aşamalar çalışma anında değişmiyor (Bulgu 6). İleride loglama ya da istatistik gibi her yere dokunan bir ihtiyaç doğarsa Decorator kullanılır. Örneğin `LoggingRadio`, bir `Radio`'yu sarar.

---

## 6. Uygulama planı

Her aşama kendi başına doğrulanabilir bir sonuçla bitiyor (Definition of Done, DoD).

### Aşama 0 — Donanım kararları ve saha doğrulaması

**Kararlar:**
- Dış ünite ESP'si bina içine alınacak mı? (Bulgu 2)
- Kanarya zil 220V ile mi çalışıyor? Öyleyse izolasyonlu, 3.3V ile tetiklenebilen (high-level trigger) bir röle modülü gerekir.
- Kapı rölesinin girişi nasıl: ortak GND mi, optokuplör mü? Ne kadar akım çekiyor?
- Dışarıdaki uzun buton hatları için RC filtre önerisi: pin ile buton arasına 1 kΩ seri direnç, pin ile GND arasına 100 nF kondansatör.
- Adaptör: kaliteli ve en az 1 A. Yüksek TX gücünde anlık akım yüzlerce mA'e çıkabiliyor, ucuz adaptörler brownout'a yol açar.

**Kod:** `Tools/RangeTest`, `EspNowRadio`'nun ilk hali, `Tools/new_site_config.ps1`.

**DoD:**
- Her komşu kat bağlantısında çerçevelerin ≥ %90'ı ulaşıyor ve RSSI ≥ −85 dBm (en az 10 dB pay).
- 2 kat ötesine bağlantı olup olmadığı ölçülüp kayıt altına alındı. Bu, bir ünite kapandığında sistemin ayakta kalıp kalmayacağını belirliyor.
- 5 kartın MAC adresi ile seçilen kanal ve TX gücü `site_config.h`'ye yazıldı.

### Aşama 1 — İskelet ve G/Ç (ağ yok) · **kod tamam, cihaz testi bekliyor**

`PressDetector` `constexpr` olduğu için testleri `Tests/StaticTests` içinde `static_assert` olarak yazıldı. Bu testler ESP32 derleyicisiyle derleme sırasında çalışıyor, bilgisayarda C++ derleyicisi gerekmiyor. Tezgâh denemesi için `Tools/IoBench` kullanılıyor.

**Kod:** kernel, io, `Bell`, `DoorOpener`, `PowerManager`, config dosyaları, `sketch.yaml`.

**DoD:**
- `PressDetector` sınır testleri geçiyor:

  | Basış | Beklenen sonuç |
  |---|---|
  | 49 ms | Ret |
  | 50 ms | Kabul |
  | 30 000 ms | Kabul |
  | 30 001 ms | Ret |
  | Açılışta basılı | Bırakılana kadar yok sayılır |
  | Bekleme süresi içinde | Ret |

- Tezgâhta: butona basınca aynı karttaki çıkış 1,5 sn aktif oluyor.
- `setup()` sonrasında boş heap miktarı sabit kalıyor.

### Aşama 2 — Ağ (şifrelemesiz, sadece tezgâhta)

**Kod:** `protocol`, `frame`, `NodeIdentity`, `EspNowRadio`'nun tamamı, `FloodRouter`, `Intercom`. `SecureChannel` aynı arayüzle ama şimdilik şifrelemesiz çalışacak.

**DoD:**
- 3 kartla (dış ünite ve 2 iç ünite) iki akış da çalışıyor.
- Aktarma yolu doğrulandı.
- Tekrar gelen kopyalar eylemi hiçbir zaman iki kez tetiklemiyor.
- Sürekli dinleme modunda kat başına gecikme ölçüldü.

### Aşama 3 — Güvenlik

**Kod:** `CcmCipher`, `ReplayWindow`, `CounterStore`, `SecureChannel`'ın tamamı.

**DoD:** Aşağıdaki senaryoların hepsi geçiyor:

| Senaryo | Beklenen sonuç |
|---|---|
| Tek biti değiştirilmiş çerçeve | Atılır |
| Başka apartmanın çerçevesi | Atılır |
| Kaydedilip tekrar gönderilen çerçeve | Atılır |
| Alıcı yeniden başlatıldıktan sonra tekrar gönderilen çerçeve | Atılır |
| Gönderici yeniden başlatıldıktan sonra yeni mesaj | Kabul edilir (sayaç rezervi sayesinde) |

### Aşama 4 — Güç

**Kod:** `EspNowRadio` uyanma penceresi modunda (`kWakeWindow`).

**DoD:**
- USB akım ölçerle her ünitede ortalama ≤ 25 mA.
- 4 katta uçtan uca gecikmenin p95 değeri ≤ 1 sn.
- Kutu içi sıcaklık ölçüldü.

### Aşama 5 — Kurulum ve saha testi

**DoD:**
- Her akış 20 kez denendi, hiç kaçırma yok.
- 1. dairenin ünitesi fişten çekildiğinde üst katların davranışı kayıt altına alındı.

---

## 7. Senin kararını bekleyenler

1. Dış ünite ESP'si bina içine taşınsın mı? (Bulgu 2)
2. Kimlik, `FLAT_ID` yerine MAC tablosundan mı gelsin? (Bulgu 4)
3. Middleware zinciri yerine sabit sıralı `SecureChannel` kabul mü? (Bulgu 6)
4. Birim testleri nasıl çalıştırılacak? Bilgisayarda C++ derleyici yok (WSL Ubuntu var ama g++ kurulu değil). İki seçenek:
   - **(a)** WSL'e `g++` ve `libmbedtls-dev` kurulur (sudo ve indirme gerekir). Core katmanı bilgisayarda test edilir.
   - **(b)** Testler cihaz üzerinde çalışır.

   Öneri: (a).
5. Proje için bir git deposu başlatılsın mı? `.gitignore` ile `site_config.h` dışarıda kalır.
6. Önceden sorulup hâlâ açık olanlar: kanarya zilin tipi, kapı rölesinin girişi.

---

## 8. Mevcut koddan geçiş

| Mevcut | Yeni |
|---|---|
| `Common/apartment_config.h` | `Common/config/site_config.h` (git dışı) + `.example.h` |
| `Common/power_saver.h` + `power_config.h` | `Common/power/power_manager.h` + `Common/config/power_config.h` + `Common/config/radio_config.h` |
| `Common/middleware.h` | Kaldırılıyor (Bulgu 6) |
| `Common/validation.h` + `input_validation.h` | Kaldırılıyor. Yerine `PressConfig` + `PressDetector` (Bulgu 7) |
| `Common/security.h` | `Common/security/*` |
| `Common/protocol.h` | `Common/net/protocol.h` (`hopLimit` çıkarıldı) |
| `Common/input_config.h` | `Common/config/input_config.h` (+ örnekleme periyodu) |
| `*/devices.h` | `*/outdoor_unit.h`, `*/indoor_unit.h` |
| `*/config.h` | `*/unit_config.h` (bekleme süreleri `PressConfig` içinde) |
| `*/hardware.h` | Aynı dosya. Dış ünite buton tablosu `{daire, pin}` çiftlerine dönüşüyor. |
| `*.ino` | 3.4'teki hali |

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
