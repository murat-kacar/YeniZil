# YeniZil — Mimari

Sürüm 3 · 05.10.2026 · Durum: **kod tamam, donanım kurulumu bekliyor**

---

## 1. Kapsam

### 1.1 Gereksinimler

- 7 daireli bina.
- **Zil paneli (dış1, bina dışında):** 7 zil butonu + panel aydınlatma LED'i. Sadece kapı ünitesiyle konuşur.
- **Kapı ünitesi (dış2, bina içinde):** Kapı rölesi tetiği (3.3V) + her daire için bir durum LED'i (7 adet) + eşleştirme butonu. Bina ağının merkezi: zil isteklerini iç ünitelere yayar, kapı açma isteklerini alır, heartbeat yayınlar.
- **İç ünite (×7):** "kapıyı aç" butonu + zil tetiği (3.3V) + bağlantı LED'i.
- Toplam 9 kart. Her ünite kendi 5V adaptöründen beslenir. Pil yok.
- Tamamen kablosuz, internet yok. ESP-NOW ile iki ayrı ağ: zil paneli bağlantısı (dış1 → dış2, doğrudan) ve bina ağı (dış2 + iç üniteler, flooding).
- Kart: ESP32-C3 Super Mini. Derleme ortamı: Arduino IDE, esp32 core 3.3.12 (ESP-IDF 5.5.5).

| Tetikleyici | Sonuç |
|---|---|
| Zil panelinde N. butona basılıp bırakıldı | Kapı ünitesi isteği alır, N. dairenin zili 1,5 sn çalar |
| N. dairede "kapıyı aç" butonuna basılıp bırakıldı | Kapı ünitesindeki röle 1,5 sn tetiklenir |
| Kapı ünitesinden heartbeat geldi (saniyede bir) | İç ünitenin bağlantı LED'i 10 ms yanar. Bağlantı yoksa LED hiç yanmaz |
| Kapı ünitesinde eşleştirme butonu 10–15 sn basılı tutulup bırakıldı | Eşleştirme modu 180 sn açılır (Karar 21) |

### 1.2 Kodun kapsamı

Depoda sadece şu beş işi yapan C++ kodu bulunur:

1. **İşin kendisi:** buton okuma, zil ve röle tetiği, ESP-NOW ile mesajlaşma ve aktarma.
2. **Güvenlik:** AES-128-CCM, tekrar koruması, kalıcı sayaçlar.
3. **Güç tasarrufu:** 80 MHz CPU, boşta WFI.
4. **Denetimler:** basış kuralları, çerçeve denetimi, ayar ve kablolama için derleme anı `static_assert`'leri.
5. **Optimizasyonlar:** tekrar gelen kopyaların şifre çözülmeden atılması gibi.

Test kodu, tezgâh (bench) programı, yardımcı betik ve seri monitör çıktısı (log) yazılmaz. Kod değişiklikleri `arduino-cli compile` ile derlenerek doğrulanır. Derleme klasörü IDE'ninkinden ayrı ve kalıcıdır (`%LOCALAPPDATA%\arduino\claude-build\`), önbellekle bir derleme ~15–20 sn sürer. Karta yükleme `arduino-cli upload` ile, sadece kullanıcı istediğinde yapılır. Kartların fiziksel denemesi kullanıcıdadır. Bir sorun çıkarsa ilgili test o zaman ayrıca istenir.

OOP, SOLID, DRY ve tasarım kalıpları, evrensel olarak benzer işlerde yerleşik standartlarda belirtildiği şekilde uygulanır (ör. IEEE 802.15.4, RFC 6347, Google C++ Style, C++ Core Guidelines).

Her şey **Arduino-first**: önce Arduino-ESP32 core'un API ve kütüphaneleri kullanılır. ESP-IDF ya da FreeRTOS çağrısı sadece Arduino karşılığı yoksa kullanılır. Hazır kütüphane yoksa ya da mevcut olanlar güvenilmezse kod elle yazılır. Hangisinin nerede kullanıldığı 5.2'de.

### 1.3 Kapsam dışı (YAGNI)

İhtiyaç doğduğunda eklenecek:

- "Kapı sadece zil çaldıktan sonra açılabilsin" kuralı
- Zaman senkronizasyonu (FTSP) ve aydınlatma LED'inin saate göre yanması: saat kaynağı yok, LED sürekli yanık
- Aydınlatma için ışık sensörü (LM393'lü dijital LDR modülü, dış1 GPIO20): "hava kararınca yan"
- Ağ anahtarını yenileme (cihaz çıkarılınca): kapı cihaz anahtarıyla korunduğu için ilk sürümde gerekmiyor
- Long Range 500 kbps: sahada menzil yeterliyse kanal yükü yarıya iner
- OTA güncelleme ve flash şifreleme
- Radyo uykusu (duty cycle) ve otomatik hafif uyku: pil gündeme gelirse, ESP-IDF'e geçişle (bkz. Karar 1)

---

## 2. Kararlar

### Karar 1 — Güç: radyo sürekli dinliyor, işlemci 80 MHz

Üniteler adaptörle besleniyor. Radyoyu uyutmak (duty cycle) ünite başına ~0,3 W kazandırıyordu, ama karşılığında her mesajın 420 ms boyunca ~42 kopya tekrarlanması, kat başına 200 ms'ye kadar gecikme ve pencere, aralık ve tekrar süresinin birbirine göre hesaplandığı bir radyo katmanı gerekiyordu. Kazanç, adaptörün kendi boştaki kaybından (0,1–0,3 W) büyük değildi.

Beklenen tüketim (C3 datasheet: alım 84 mA, 80 MHz'de işlemci boşta 13–18 mA): ünite başına ortalama ≈ 85–100 mA, ≈ 0,45 W. 300 mA'lik adaptör için yeterli.

**Karar:** Modem uykusu kapalı, radyo sürekli dinliyor. İşlemci 80 MHz'de çalışıyor (Wi-Fi'ın izin verdiği en düşük frekans) ve boşta WFI ile bekliyor. Otomatik hafif uyku bu core'da kapalı (`CONFIG_PM_ENABLE`). Pil ya da güneş paneli gündeme gelirse duty cycle, hafif uyku ve senkronlu pencereler birlikte, ESP-IDF'e geçerek ele alınır.

### Karar 2 — Röle ve bina ağının şifresi bina içinde

Röle tetik kablosu bina dışındaki kutuda durursa kutuyu açan biri tetik kablosunu 3.3V'a değdirip kapıyı açabilir. Bina ağının şifresi dışarıdaki kartta durursa biri flash'ı okuyup geçerli bir "kapıyı aç" mesajı üretebilir. ESP32-C3'ün flash'ı şifresiz, esptool ile birkaç dakikada okunur.

**Karar:** Röle ve onu süren kapı ünitesi (dış2) bina içinde, kapının güvenli tarafında. Dışarıdaki zil panelinde (dış1) sadece butonlar ve kendi bağlantı şifresi var. Bina ağının şifresi orada yok (Karar 19). Kutuyu açıp flash'ı okuyan biri en fazla sahte zil çaldırabilir, kapıyı açamaz. Bu, erişim kontrol sistemlerindeki yaygın kurulum kuralının (kilidi süren eleman güvenli tarafta) kablosuz karşılığı. Bedeli: dış1 yağmura, güneşe ve ısınmaya dayanıklı bir kutuda olmalı.

### Karar 3 — Alıcı sayacı kalıcı

Tekrar koruma durumu yalnızca RAM'de tutulursa şu saldırı mümkün olur:
1. Saldırgan bir "kapıyı aç" mesajını havadan kaydeder.
2. Kapı ünitesinin fişini çekip takar. Tekrar koruma durumu sıfırlanır.
3. Kaydettiği mesajı tekrar gönderir, kapı açılır.

**Karar:** Kendine gelen ve eyleme dönüşecek her mesajın sayacı, gönderen kartın MAC'i anahtar olarak kullanılarak **eylemden önce** NVS'ye yazılıyor. Yazılamazsa eylem yapılmıyor. Açılıştan sonra bir karttan ilk doğrulanmış çerçeve geldiğinde o kartın tekrar penceresi "kayıtlı sayaca kadar hepsi görüldü" durumuyla başlıyor. NVS aşınmayı dengelediği için flash ömrü sorun değil: günde 50 olay × 9 ünite, 100 bin silme döngüsünün çok altında kalıyor.

### Karar 4 — Kimlik yüklemede verilir, nonce MAC'ten üretilir

Bütün atamalar firmware'e yükleme sırasında veriliyor. Kurulumda cihazla ayrıca bir işlem yapılmıyor.

- **Kapı ünitesi (dış2):** Kimliği her zaman 0 (`kDoorUnitId`).
- **Zil paneli (dış1):** Kimliği her zaman `kBellPanelId` (0xFE, daire numaralarının dışında). Hangi butonun hangi daireyi çaldıracağı `bell_panel/hardware.h` içindeki `{daire, pin}` tablosunda.
- **İç ünite:** Daire numarası `indoor/indoor_config.h` içindeki `kFlatId` (1..`kFlatCount`). Her iç üniteye yüklemeden önce ayarlanıyor. Aralık dışı bir değer derlenmiyor.

Elle verilen kimlikte iki kartın yanlışlıkla aynı numarayla yüklenmesi mümkün. Nonce kimlikten üretilseydi bu, aynı nonce demek olurdu ve AES-CCM'in güvenliği çökerdi.

**Karar:** Gönderen kartın fabrika MAC adresi her çerçevede taşınıyor. Nonce MAC + sayaçtan üretiliyor (IEEE 802.15.4 CCM* yöntemi). Tekrar koruması ve kalıcı sayaçlar da göndereni MAC'e göre izliyor. Bunun iki sonucu var:
- Numaralar çakışsa bile nonce tekrarlanmıyor, çakışmanın etkisi sadece işlevsel kalıyor.
- Bir kart değiştirildiğinde yeni kartın sayacı sıfırdan başlasa bile mesajları reddedilmiyor.

Kullanıcının MAC'le ilgili yapacağı bir şey yok.

### Karar 5 — TTL yok

Tekrar koruması her kaynak için kayan pencereyle çalışıyor. Bu yüzden bir düğüm aynı mesajı en fazla bir kez aktarıyor ve flooding'in sonlanması garanti. `hopLimit` alanı yok, bu sayede başlığın tamamı imzalı.

### Karar 6 — Sabit sıralı işlem hattı

Güvenlik adımlarının sırası kritik. Pencere doğrulamadan önce ilerletilirse sahte yüksek sayaçlar gerçek mesajları engelleyebilir (DoS).

**Karar:** Adımlar `SecureChannel` içinde sabit sırayla çalışıyor (Pipes and Filters kalıbının statik biçimi). Her adım ayrı bir sınıf: `frame`, `CcmCipher`, `ReplayWindow`, `CounterStore`. Sıra çalışma anında değiştirilemiyor.

### Karar 7 — Basış kuralları tek yapıda

Buton denetimlerinin hepsi bir basışın sayısal sınırları: en kısa süre, en uzun süre, bekleme süresi. Zil panelindeki daire başına zil beklemesi ile iç ünitedeki kapı isteği beklemesi aynı ihtiyaç.

**Karar:** Hepsi `PressSettings` yapısında. Ayrı kural sınıfları yok (Rule of Three), aynı mekanizma iki ünitede de kullanılıyor (DRY).

### Karar 8 — 64-bit zaman

`millis()` 49,7 günde taşar. **Karar:** Zaman `esp_timer` tabanlı 64-bit `monotonicMs()` fonksiyonundan alınıyor.

### Karar 9 — Butonlar örneklemeyle okunuyor

**Karar:** Butonlar 5 ms'de bir okunuyor (Ganssle: 1–5 ms). İşlemci otomatik uykuya geçmediği için (Karar 1) ek maliyeti yok. Kesme ve uyku seviyesi değişikliğinden gelen karmaşıklık da ortadan kalkıyor.

### Karar 10 — Her çerçeve 3 kez gönderiliyor

ESP-NOW yayınları onaysız: gönderen çerçevenin ulaşıp ulaşmadığını bilmiyor. Tek bir kopyanın bir kat geçişinde ulaşma oranı %90 ise, tek gönderimde kat başına kaçırma %10, 4 katta ≈ %35 olur.

**Karar:** Her çerçeve 20 ms arayla 3 kez gönderiliyor, aktaranlar da aynı şekilde (BLE Mesh'in ağ tekrarı gibi). Kat başına kaçırma 0,1³ = %0,1, 4 katta ≈ %0,4. Radyo sürekli dinlediği için mesaj genelde ilk kopyada yakalanıyor, gecikme kat başına birkaç ms.

### Karar 11 — Yüklemeden önce değişen değerler tek dosyada

**Karar:** Yüklemeden önce değiştirilen her şey her ünitenin kendi config dosyasında duruyor. Başka hiçbir dosyaya dokunulmuyor.

- `bell_panel/bell_panel_config.h`: zil paneli bağlantısının kimliği ve şifresi.
- `door_unit/door_unit_config.h`: bina ağının kimliği ve şifresi, zil paneli bağlantısının kimliği ve şifresi.
- `indoor/indoor_config.h`: bina ağının kimliği ve şifresi, daire numarası.

Bina ağının kimliği ve şifresi kapı ünitesinde ve yedi iç ünitede aynı olmalı. Bağlantının kimliği ve şifresi zil panelinde ve kapı ünitesinde aynı olmalı. Dosyalar git'te. Kimlik ya da şifre sıfır bırakılırsa, iki ağın kimliği ya da şifresi aynıysa, daire numarası aralık dışındaysa program derlenmiyor.

Kapı ünitesinin ağdaki numarası (0) ve ünitelerin diğer ayarları (darbe süreleri, basış kuralları) `bell_panel.h` / `door_unit.h` / `indoor.h` içinde. Daire sayısı, kanal ve TX gücü gibi bina geneli ayarlar `common/config` altında.

### Karar 12 — AES-128-CCM, 8 bayt etiket

IEEE 802.15.4, Zigbee, Thread ve BLE'nin kullandığı standart. mbedTLS core'da hazır ve C3'te donanım hızlandırmalı: `CONFIG_MBEDTLS_HARDWARE_AES=y`, `CONFIG_MBEDTLS_CCM_C=y`. Kapı açma kritik olduğu için etiket 4 değil 8 bayt.

### Karar 13 — Gönderme sayacı rezervi ve sınırı

Gönderici her mesajda sayacı flash'a yazmıyor. 1000'lik bir rezervin sonunu yazıyor, yeniden başlayınca oradan devam ediyor (OpenThread `STORE_FRAME_COUNTER_AHEAD`). Rezerv flash'a yazılamazsa ya da sayaç 32 bitin sonuna gelirse gönderim duruyor. Nonce tekrarlanmasın diye bu durumda anahtarların değişmesi gerekiyor. Aynı nedenle Arduino IDE'de "Erase All Flash Before Sketch Upload" kapalı kalmalı: açılırsa kayıtlı sayaçlar silinir, anahtarlar da değiştirilmelidir.

Kartta tek gönderme sayacı var (`TxCounter`), karttaki bütün ağlar onu paylaşıyor. İki ağın iki ayrı sayacı olsaydı ikisi aynı NVS kaydını okuyup yazacak ve yeniden başlamada bir sayaç tekrar kullanılabilecekti. Tek sayaçla nonce (MAC + sayaç) hangi anahtarla olursa olsun hiç tekrarlanmıyor.

### Karar 14 — Radyo ayarları radyonun işi

TX gücü, Long Range ve kanal `RadioSettings` içinde, `EspNowRadio` tarafından uygulanıyor. Tekrarlı gönderimin ayarları `BurstSettings` içinde, `BurstSender` tarafından uygulanıyor. `PowerManager` sadece CPU frekansını yönetiyor. Böylece bileşenler arasında başlatma sırası bağımlılığı kalmıyor.

### Karar 15 — Watchdog

`CONFIG_ESP_TASK_WDT_TIMEOUT_S=5`. **Karar:** Döngü görevi watchdog'a bağlı ve olay beklemesi en fazla 1 sn. Kod kilitlenirse cihaz yeniden başlıyor. Röle pini pull-down'da olduğu için yeniden başlama kapıyı açmıyor.

### Karar 16 — Kopyalar şifre çözülmeden atılıyor (optimizasyon)

Her mesaj gönderenden ve her aktarıcıdan 3'er kopya geliyor. Tekrar penceresi salt okunur olarak AES'ten **önce** kontrol ediliyor, görülmüş kopya şifre çözülmeden atılıyor. Pencere sadece doğrulanmış çerçeveyle ilerlediği için bu sıralama Karar 6'daki DoS riskini doğurmuyor.

### Karar 17 — Log yok

**Karar:** Kodda seri port ve log yok. Hatalar dönüş değeriyle bildiriliyor, başlayamayan bileşen kapalı kalıyor. Örneğin NVS açılamazsa ya da anahtar yüklenemezse ağ açılmıyor.

### Karar 18 — Bağlantı göstergesi: heartbeat

Flooding ağında sürekli bir bağlantı yok, mesaj sadece olay olunca gidiyor. İç ünitenin "bağlıyım" diyebilmesi için düzenli bir sinyal gerekiyor.

**Karar:** Kapı ünitesi her 1 sn'de bir, açılışta da hemen, herkese (`kAllUnitsId`) doğrulanmış bir heartbeat yayınlıyor. Her ünite bunu teslim alıp aktarıyor, böylece üst katlara da ulaşıyor. İç ünite her heartbeat'te bağlantı LED'ini 10 ms yakıyor, LED'in ne zaman yanacağına kapı ünitesi karar veriyor. Bağlıyken LED saniyede bir kısa yanıp söner. Bağlantı yoksa hiç yanmaz. İç ünitede zaman aşımı ya da "bağlı mıyım" durumu yok, kopukluk ilk kaçan yanıp sönmede görülüyor. Heartbeat bir eylem değil. Bu yüzden sayacı flash'a yazılmıyor, flash aşınmıyor. Maliyeti her ünitenin saniyede bir 3 kopya göndermesi: 8 ünitede saniyede ~24 kısa çerçeve, kanalın ~%5'i. LED kapı ünitesine olan bağlantıyı gösteriyor. Zil paneli ile kapı ünitesi arasındaki bağlantının göstergesi yok.

### Karar 19 — İki ağ: zil paneli bağlantısı ve bina ağı, kapı ünitesi ağ geçidi

Zil paneli bina dışında (Karar 2). Tek ortak şifre olsaydı panelin flash'ını okuyan biri kapıyı açabilirdi.

**Karar:** İki ayrı ağ var, her birinin kendi kimliği ve şifresi var:
- **Zil paneli bağlantısı:** Zil paneli + kapı ünitesi. Panel sadece "N. dairenin zili" isteği gönderiyor. Kapı ünitesi bu bağlantıdan başka bir şey kabul etmiyor: kaynak zil paneli, tip zil, hedef geçerli bir daire olmalı. Aktarma yok.
- **Bina ağı:** Kapı ünitesi + iç üniteler, flooding (önceki tasarımla aynı). Zil paneli kaynaklı çerçeve bu ağda reddediliyor.

Kapı ünitesi iki ağın arasında ağ geçidi: panelden gelen isteği doğruluyor, kalıcı kaydını yapıyor, zili kendi MAC'i, kendi sayacı ve bina ağının şifresiyle yeniden gönderiyor. Bu, IEEE 802.15.4 MAC güvenliğindeki hop-by-hop yaklaşım: her bağlantı kendi anahtarıyla korunuyor, ara düğüm yeniden imzalıyor. Panel çerçevesinin hedef alanında çalınacak daire var, çerçeveyi alan ise ağ geçidi. IP'de son hedef adresi ile ağ geçidinin ilişkisi de böyle. Bu sayede çerçeve biçimi değişmedi.

Sonuçları:
- Panel ele geçirilirse saldırgan en fazla sahte zil çaldırabilir. Kapı açmak bina ağının şifresini gerektiriyor ve bu şifre panelde yok.
- İç ünitelerin gözünden değişen bir şey yok. Zil ve heartbeat yine 0 numaralı üniteden geliyor, kapı açma isteği yine 0'a gidiyor.
- İki ağın kimliği ya da şifresi aynı olursa ayrımın anlamı kalmaz. Kapı ünitesinde bu durum `static_assert` ile derlenmiyor.

### Karar 20 — Radyo çerçeveleri ağlara dağıtıyor

Kapı ünitesinde tek radyonun üstünde iki ağ çalışıyor. Alma kuyruğunu tek bir ağ boşaltsaydı öteki ağın çerçeveleri kaybolurdu.

**Karar:** `EspNowRadio` kuyruktaki her çerçeveyi kendisine bağlı bütün ağlara (`FrameReceiver`) veriyor (Observer). Her ağ kendi constructor'ında radyoya bağlanıyor. Liste radyonun içinde, ağların kendi bağlantı alanlarıyla kuruluyor (intrusive list, heap yok). Her ağ öteki ağın çerçevesini ilk ucuz denetimde, ağ kimliğine bakarak atıyor. Radyo, uzunluğu protokol çerçevesinden (26 bayt) farklı veriyi kuyruğa hiç koymuyor. Radyo, tekrarlı gönderim ve sayaçlar karttaki ağlar arasında ortak, hepsi `RadioStack` içinde kuruluyor.

### Karar 21 — Eşleştirme modu: kutudaki BOOT butonu, 30 sn pencere

Eşleştirme her zaman açık olsaydı menzildeki herkes her an katılmayı deneyebilirdi.

**Karar:** Eşleştirme modu, kartın kendi BOOT butonuna (GPIO9) 10–15 sn basılı tutulup bırakılınca açılıyor. Bu, dış2'de ve dış1'de aynı; iç ünitede ise kapı butonu kullanılıyor. Kartlar vidalı kutunun içinde olduğu için **eşleştirme yetkisi kutuyu açan kişide.** 10 sn'nin altı kazara basış, 15 sn'nin üstü sıkışmış buton sayılıyor (Karar 7).

Mod 30 sn işlem yapılmazsa kendiliğinden kapanıyor. Süre her basışta baştan başlıyor, buton basılıyken dolmuyor. Eşleştirme apartman girişinde, iç ünite dış2'nin yanındayken yapıldığı için 30 sn yetiyor.

BOOT butonu kart yeniden başlarken basılı tutulursa kart yükleme modunda açılır. Butona sadece kutuyu açan kişi erişebildiği için bu kabul edilen bir risk. Böylece GPIO20 dış1'de ve dış2'de boşalıyor. Ayrıntı: 8.3.

> **Kod durumu:** Kodda eşleştirme butonu henüz GPIO20'de, pencere 180 sn ve tek seferlik. v3 koduyla düzeltilecek.

### Karar 22 — Bina kimliği dış2'de doğar, config dosyası yok

Bütün iç ünitelerin ve dış1'lerin yazılımı aynı olacak. Şifre yazılımda olsaydı her binada ve raftaki her cihazda aynı olurdu, binalar birbirinden ayrılamazdı.

**Karar:** Dış2 ilk açılışta donanım rastgele sayı üreteciyle bina ağının kimliğini ve şifresini, zil paneli bağlantısının kimliğini üretiyor, NVS'de saklıyor ve radyo kanalını seçiyor (Karar 28). İki binanın şifresinin çakışma olasılığı 2⁻¹²⁸. İç ünite ve dış1 bu bilgileri eşleşmede alıyor.

Ünite klasörlerindeki config dosyaları kalkıyor. Ürün ayarları (`common/config/`) kalıyor.

Dış2 bozulur ya da flash'ı silinirse bina yeni bir kimlikle başlar. Bütün iç üniteler ve dış1 yeniden eşleşir, bu kabul edildi. Ayrıntı: 8.2.

### Karar 23 — Şifre teslimi: X25519 + HKDF, yan yana eşleşme

**Karar:**
- Eşleşen iki kart geçici X25519 anahtar çifti üretiyor ve açık anahtarları değiştiriyor (ECDH).
- Ortak sırdan HKDF-SHA256 ile iki anahtar türetiliyor:
  - **Oturum anahtarı:** eşleşme mesajlarını şifreliyor.
  - **Cihaz anahtarı:** kalıcı. Havada hiç taşınmıyor, iki taraf da kendisi türetiyor.
- Dış2 ağ şifresini oturum anahtarıyla şifreleyip gönderiyor. Havayı dinleyen biri bu sırrı hesaplayamaz.

Araya girmeye (MITM) karşı önlemler:
- İki tarafta fiziksel buton ve 30 sn pencere.
- **Tek aday:** Dış2 pencere içinde ikinci bir aday görürse, iç ünite ikinci bir cevap görürse eşleşme iptal ediliyor. Havadan yayın yapıldığı için araya giren saldırgan gerçek cihazın sinyalini bastıramaz.
- **Yakınlık:** Sinyali zayıf aday kabul edilmiyor, eşleşme yan yana yapılıyor.

X25519, HKDF ve AES-CCM core'un içindeki mbedTLS'te hazır (`CONFIG_MBEDTLS_ECDH_C`, `CONFIG_MBEDTLS_ECP_DP_CURVE25519_ENABLED`, `CONFIG_MBEDTLS_HKDF_C`). Ayrıntı: 8.5.

### Karar 24 — Cihaz anahtarı ve iki katmanlı imza

Ortak ağ şifresiyle tek bir cihazın yetkisi iptal edilemez: sökülen, bozulan ya da çalınan iç ünite şifreyi taşımaya devam eder.

**Karar:** Zigbee'nin iki katmanlı modeli uygulanıyor.
- **Ağ anahtarı (dış katman, NWK gibi):** Her çerçevede var. Aradaki üniteler bununla doğrulayıp aktarıyor.
- **Cihaz anahtarı (iç katman, APS gibi):** Kapı açma, "buradayım" ve "çaldım" mesajlarında ikinci bir 8 baytlık etiket. Uçtan uca: sadece dış2 ile o cihaz biliyor.

Dış2 bir kanalı silince o cihaz kapıyı açamaz, "buradayım" diyemez. Ağ şifresi sızarsa saldırgan en fazla sahte zil ya da heartbeat üretebilir. Dış1'in cihaz anahtarı zil paneli bağlantısının anahtarı oluyor. Ayrıntı: 8.8.

### Karar 25 — Login: 10 sn'de bir "buradayım"

**Karar:** Eşleşmiş her iç ünite 10 sn'de bir dış2'ye cihaz anahtarıyla imzalı "buradayım" gönderiyor. Bu aynı zamanda login bildirimi. Dış2 son 30 sn içinde mesaj alamadığı cihazı çevrimdışı sayıyor ve o dairenin LED'ini söndürüyor. Normal modda sönük LED arıza ya da boş kanal anlamına geliyor.

### Karar 26 — Zil onayı uçtan uca

Ziyaretçi zilin çalıp çalmadığını bilmiyordu. ESP-NOW yayınları onaysız (Karar 10).

**Karar:**
- Daire, zili tetikleyince dış2'ye "çaldım" gönderiyor. Dış2 bunu bağlantı üzerinden dış1'e iletiyor.
- Dış1 2 sn içinde onay alırsa aydınlatma LED'i 5 kez kısa yanıp sönüyor, almazsa 1 sn sönüp geri yanıyor.
- Onay, iç ünitenin zil çıkışını tetiklediğini kanıtlıyor. Zilin kablosunun ya da kendisinin sağlam olduğunu kanıtlamıyor.

Bir zil ve onayı kanalda bir kez ~0,13 sn yer kaplıyor.

### Karar 27 — Aktarmayı bastırma (Trickle)

Bugünkü kurala göre her mesajı bütün üniteler 3'er kopya aktarıyor. Periyodik mesajlar da her üniteden çıktığı için kanal yükü daire sayısının karesiyle büyüyor: 7 dairede ~%9, 30 dairede ~%95.

**Karar:** Thread'in yayın aktarmasındaki yöntem uygulanıyor (Trickle, RFC 6206 / MPL, RFC 7731).
- İlk gönderen 3 kopya gönderiyor.
- Aktarıcı rastgele bir süre bekliyor. O sürede aynı mesajı başka aktarıcılardan k kez duyduysa aktarmıyor.
- Sadece aktarıcıların kopyaları sayılıyor. İlk gönderenin kopyaları sayılsaydı onun menzilindeki herkes susar, uzaktaki katlara mesaj gitmezdi.

Başlangıç değerleri: k = 2, aktarıcı 2 kopya. Aktarıcı tek kopya gönderseydi tek aktarıcının ulaştığı bir katta kayıp oranı %10'a çıkardı. Değerler sahada ayarlanacak.

Kanal yükü 7 dairede ~%3–4'e, 30 dairede ~%8–9'a iniyor ve daire sayısıyla karesel büyümüyor. Karar 10'daki "aktarıcı 3 kopya" kuralının yerine geçiyor.

### Karar 28 — Otomatik kanal seçimi

Config kalkınca bütün binalar aynı kanalda olurdu. Komşu binaların yükleri toplanırdı.

**Karar:**
- Dış2 ilk açılışta 1, 6 ve 11 numaralı kanallardan (birbiriyle örtüşmeyenler) en sakinini seçiyor: en az ve en zayıf erişim noktası olanı, `WiFi.scanNetworks`.
- Kanal binanın kimliğinin parçası.
- Eşleşen kart dış2'yi bu üç kanalı sırayla deneyerek buluyor (Zigbee aktif tarama). Dış2 eşleşme sırasında kendi kanalından ayrılmıyor, bina trafiği kesilmiyor.
- Kanal eşleşme cevabında bildiriliyor ve NVS'ye yazılıyor.

Komşu binalar üç kanala dağılıyor.

---

## 3. Mimari

### 3.1 Katmanlar

```
┌──────────────────────────────────────────────────────────────┐
│ Sketch     *.ino                bağlamalar: aksiyon → sonuç     │
├──────────────────────────────────────────────────────────────┤
│ Unit       bell_panel.h         composition root:               │
│            door_unit.h          nesneleri kurar, ayar verir     │
│            indoor.h                                             │
├──────────────────────────────────────────────────────────────┤
│ App        RadioStack · Intercom · PanelLink · Bell · DoorOpener │
│            (ağları kurar, alan dili)                            │
├──────────────────────────────────────────────────────────────┤
│ Services   FloodRouter · BurstSender · SecureChannel · TxCounter │
│            ButtonGroup · Button · PulseOutput                   │
├──────────────────────────────────────────────────────────────┤
│ Platform   EspNowRadio · BroadcastPeer · CcmCipher              │
│            CounterStore · PowerManager                          │
│            (radyo, şifreleme, flash, CPU sürücüleri)            │
├──────────────────────────────────────────────────────────────┤
│ Core       PressDetector · ReplayWindow · frame · protocol      │
│            (saf C++: Arduino/IDF include etmez)                 │
├──────────────────────────────────────────────────────────────┤
│ Kernel     EventLoop · Component · Handler · monotonicMs()      │
└──────────────────────────────────────────────────────────────┘
```

Kurallar:
- Bağımlılık sadece aşağı doğru olabilir.
- Config her katmana değer verir ama hiçbir katmana bağımlı değildir. Ayar tipleri de (`RadioSettings`, `BurstSettings`, `PressSettings`) config katmanında, `*_settings.h` dosyalarında. `NetworkCredentials` ağ kimliği ve şifre tipine dayandığı için güvenlik katmanında.
- Bütün kod `yenizil` isim alanında. Ayarlar `yenizil::config`, kablolama `yenizil::pins`, kart bilgisi `yenizil::board`, çerçeve biçimi `yenizil::frame` altında.
- Core katmanındaki dosyalar platforma dokunmaz.

### 3.2 Klasör yapısı (özelliğe göre paketleme)

```
YeniZil/
├── bell_panel/                     zil paneli (dış1), bina dışında
│   ├── bell_panel.ino              bağlamalar
│   ├── bell_panel.h                composition root + ünite ayarları
│   ├── hardware.h                  kablolama: zil butonları, aydınlatma LED'i
│   └── bell_panel_config.h         yüklemeden önce: bağlantı kimliği ve şifresi
├── door_unit/                      kapı ünitesi (dış2), bina içinde
│   ├── door_unit.ino
│   ├── door_unit.h
│   ├── hardware.h                  kablolama: kapı rölesi, daire durum LED'leri, eşleştirme butonu
│   └── door_unit_config.h          yüklemeden önce: bina ağı ve bağlantının kimlik ve şifreleri
├── indoor/
│   ├── indoor.ino
│   ├── indoor.h
│   ├── hardware.h
│   └── indoor_config.h             yüklemeden önce: bina ağının kimliği ve şifresi, daire numarası
├── common/
│   ├── config/
│   │   ├── building_config.h       daire sayısı
│   │   ├── radio_settings.h        RadioSettings, BurstSettings tipleri
│   │   ├── radio_config.h          kanal, güç, Long Range, tekrar sayısı ve aralığı
│   │   ├── press_settings.h        PressSettings tipi
│   │   ├── input_config.h          basış sınırları
│   │   ├── link_config.h           heartbeat aralığı
│   │   └── power_config.h          CPU frekansı
│   ├── kernel/   callback.h · clock.h · component.h · polling_component.h · periodic_timer.h · event_loop.h · byte_order.h · enum_value.h · static_checks.h
│   ├── io/       board_pins.h · digital_pin.h · id_pin.h · press_detector.h · button.h · button_group.h · pulse_output.h
│   ├── net/      protocol.h · frame.h · nodes.h · frame_receiver.h · broadcast_peer.h · esp_now_radio.h · burst_sender.h · flood_router.h
│   ├── security/ ccm_cipher.h · replay_window.h · counter_store.h · tx_counter.h · network_credentials.h · secure_channel.h
│   ├── power/    power_manager.h
│   └── app/      radio_stack.h · intercom.h · panel_link.h · pairing.h · bell.h · door_opener.h
├── docs/ARCHITECTURE.md
└── .gitignore                      build/
```

### 3.3 Sınıflar

| Sınıf | Tür | Sorumluluk | Public arayüz |
|---|---|---|---|
| `Handler<Args...>` / `callIfSet()` | kernel | Olay işleyicisi tipi (düz fonksiyon işaretçisi) ve "bağlıysa çağır" yardımcısı | `callIfSet(handler, args...)` |
| `monotonicMs()` | kernel | 64-bit monoton zaman (ms), `esp_timer` | `uint64_t monotonicMs()` |
| `Component` | soyut | Güncellenen her şeyin ortak arayüzü. Constructor'da kendini zincire ekler (intrusive list, heap yok). | `begin()`, `update(nowMs)`, `nextDeadlineMs()` |
| `PollingComponent` | soyut | Sabit periyotla örnekleme (Template Method). `Button` ve `ButtonGroup` ortak zamanlamayı buradan alır. | `poll(nowMs)` (korumalı) |
| `PeriodicTimer` | kernel | Sabit aralıkla olay, ilk olay açılışta (`PollingComponent`) | `onTick(Handler<>)` |
| `EventLoop` | kernel | Bileşenleri başlatır ve günceller. En yakın zamana ya da bildirime kadar bloklanır (≤ 1 sn). Watchdog'u açar. Tek örneği ünite dosyasında kurulur, radyoya constructor'dan verilir. | `begin()`, `update()`, `notify()` |
| `writeLe` / `readLe` | kernel | Tamsayıyı little-endian yazar/okur (`std::bit_cast`) | — |
| `allUnique()` | kernel | Tabloda tekrar var mı, derleme zamanında | `consteval bool allUnique(items, key)` |
| `setupInput()` / `isActive()` / `setupOutput()` / `writeOutput()` | io | Aktif seviyeye göre pin okuma ve yazma. Çıkış titremeden pasif açılır. | — |
| `board::isSafeGpio()` | io | Super Mini'de açılışı etkilemeyen pinler: 0, 1, 3, 4, 5, 6, 7, 10, 20 | `constexpr bool isSafeGpio(pin)` |
| `PressDetector` | core | Seviye ve zamandan geçerli basışı çıkarır: süre sınırları, bekleme süresi, açılışta basılı butonu bırakılana kadar yok sayma | `bool update(pressed, nowMs, settings)` |
| `Button` | servis | Tek buton: 5 ms örnekleme + `PressDetector` | `onPress(Handler<>)` |
| `IdPin<Id>` | io | Kimlikli pin bağlantısı (buton ya da LED tablosunun satırı): daire numarası + pin | — |
| `ButtonGroup<Id, N>` | servis | Kimlikli N buton (`IdPin` tablosu), olay kimlikle gelir | `onPress(Handler<Id>)` |
| `PulseOutput` | servis | Belirli süre aktif kalan çıkış. Aktifken gelen tetik yok sayılır. Açılışta titremeden pasife çekilir. | `activate()` |
| `Bell` / `DoorOpener` | app | Alan dilinde eylem (`PulseOutput` içerir) | `ring()` / `open()` |
| `protocol` | core | `NodeId`, `NetworkId`, `FrameCounter`, `MacAddress`, `MessageType`, `Message`, sürüm. Ünite kimlikleri (`kDoorUnitId`, `kBellPanelId`, `kAllUnitsId`) ve hangi tipin herkese gittiği burada. | `isKnownMessageType()`, `isBroadcast()`, `matchesAddressing()` |
| `frame` | core | Çerçeveyi bayt bayt yazar ve okur, nonce üretir, alanlara `std::span` verir | `encodeHeader()`, `decodeHeader()`, `nonce()`, `header()`, `payload()`, `tag()` |
| `nodes` | denetim | Daire numarası, ünite ve hedef denetimleri | `isFlatId()`, `isNode()`, `isDestination()` |
| `FrameReceiver` | soyut | Radyodan çerçeve alan ağın arayüzü (Observer). Radyonun alıcı listesindeki bağlantı alanını taşır. | `receive(bytes)` |
| `BroadcastPeer` | platform | Arduino `ESP_NOW_Peer`'den türeyen yayın eşi, Long Range hızıyla | `begin()`, `send(bytes)` |
| `EspNowRadio` | platform | Wi-Fi/ESP-NOW başlatma, kanal, TX gücü, Long Range, sürekli dinleme, tek kopya gönderim. Alma kuyruğundaki çerçeveleri bağlı bütün ağlara dağıtır (Karar 20). | `attach(receiver)`, `send(bytes)`, `isReady()`, `ownMac()` |
| `BurstSender` | servis | Çerçeveyi 20 ms arayla 3 kez gönderir. Aktarmada kısa rastgele bekleme. | `send(bytes)`, `relay(bytes)` |
| `CcmCipher` | platform | AES-128-CCM (mbedTLS) | `bool begin()`, `bool seal(...)`, `bool open(...)` |
| `ReplayWindow` | core | 64'lük kayan pencere | `isFresh()`, `markSeen()`, `restore()` |
| `CounterStore` | platform | NVS (`Preferences`): gönderme sayacı rezervi, gönderen MAC başına son eylem sayacı. `begin()` birden çok kez çağrılabilir. | `begin()`, `loadTxReserve()`, `saveTxReserve()`, `loadRxCounter()`, `saveRxCounter()` |
| `TxCounter` | servis | Kartın tek gönderme sayacı, rezervle (Karar 13). Karttaki ağlar paylaşır. | `optional<FrameCounter> next()` |
| `NetworkCredentials` | güvenlik | Bir ağın kimliği ve şifresi | `isAssigned()`, `isSeparate()` |
| `SecureChannel` | servis | Tek ağın çerçeveleri, sabit sıra (3.6). Giden çerçeveyi şifreler ve imzalar. Tekrar penceresi gönderen MAC başına. | `bool begin(mac)`, `optional<Bytes> seal(src, dst, type)`, `optional<Message> open(bytes)`, `bool commit(message)` |
| `FloodRouter` | servis | Bina ağında gelen çerçeveyi süzer, kendine geleni kalıcı kayıttan sonra teslim eder, herkese gideni teslim edip aktarır, gerisini **değiştirmeden** aktarır | `send(dst, type)`, `on(type, Handler<>)` |
| `RadioStack` | app | Kartın ortak radyo katmanını kurar: radyo, tekrarlı gönderim, sayaç kaydı, gönderme sayacı | `radio()`, `bursts()`, `store()`, `txCounter()` |
| `Intercom` | app (Facade) | Bina ağının şifrelemesini, güvenli kanalını ve yönlendiricisini kurar ve gizler, alan dilinde işlemler sunar | `ringFlat(NodeId)`, `requestDoorOpen()`, `broadcastHeartbeat()`, `onRing()`, `onDoorOpenRequest()`, `onHeartbeat()` |
| `PanelLink` | app | Zil paneli bağlantısı (Karar 19): panel tarafında zil isteği gönderir, kapı ünitesi tarafında sadece panelin zil isteğini kabul eder. Aktarma yok. | `requestRing(NodeId)`, `onRingRequest(Handler<NodeId>)` |
| `Pairing` | app | Eşleştirme modu: süreli pencere, süre dolunca kendiliğinden kapanır (Karar 21) | `open()`, `isOpen()` |
| `PowerManager` | platform | CPU frekansı | `begin()` |

**Tasarım kuralları:**
- Constructor'lar sadece ayarları saklar, donanıma dokunmaz. Donanım `begin()` içinde başlatılır, çünkü global nesnelerin constructor'ları Arduino hazır olmadan çalışır. Ağların radyoya bağlanması (`attach`) sadece bir işaretçi saklar, bu yüzden constructor'da yapılır.
- Bağımlılıklar constructor'dan referansla verilir (dependency injection). Global nesneye doğrudan erişen sınıf yok. Nesneleri composition root (`bell_panel.h`, `door_unit.h`, `indoor.h`) kurar. Ortak radyo katmanını `RadioStack`, her ağın kendi parçalarını `Intercom` ve `PanelLink` kendi içinde kurar. Ünite sadece ayarları verir.
- Sanal fonksiyon sadece `Component` ve `FrameReceiver` hiyerarşilerinde ve Arduino'nun `ESP_NOW_Peer` sınıfında var. İkisi de farklı tipteki nesneleri tek listede tutuyor. Tek uygulaması olan şey için arayüz yazılmıyor (YAGNI).
- Kalıtım yerine composition tercih ediliyor: `Bell`, bir `PulseOutput` **içeriyor**, ondan türemiyor.

### 3.4 Sketch'ler

```cpp
#include "bell_panel.h"  // Zil paneli nesneleri

using namespace yenizil;  // Proje isim alanı

void setup() {
  flatButtons.onPress([](NodeId flat) { panelLink.requestRing(flat); });  // N. daire butonu -> kapı ünitesine N. dairenin zil isteği
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
```

```cpp
#include "door_unit.h"  // Kapı ünitesi nesneleri

using namespace yenizil;  // Proje isim alanı

void setup() {
  panelLink.onRingRequest([](NodeId flat) { intercom.ringFlat(flat); });  // Zil panelinden N. daire isteği -> N. dairenin zili
  intercom.onDoorOpenRequest([] { doorOpener.open(); });                  // Kapı açma isteği -> kapı açılır
  heartbeatTimer.onTick([] { intercom.broadcastHeartbeat(); });           // Periyot doldu -> "buradayım" yayını
  pairingButton.onPress([] { pairing.open(); });                          // Eşleştirme butonu 10-15 sn basılı tutulup bırakıldı -> eşleştirme modu açılır
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
```

```cpp
#include "indoor.h"  // İç ünite nesneleri

using namespace yenizil;  // Proje isim alanı

void setup() {
  openDoorButton.onPress([] { intercom.requestDoorOpen(); });  // Kapıyı aç butonu -> kapı ünitesine istek
  intercom.onRing([] { bell.ring(); });                         // Zil isteği -> zil çalar
  intercom.onHeartbeat([] { linkLed.activate(); });             // Kapı ünitesinden "buradayım" -> bağlantı LED'i kısa yanar
  eventLoop.begin();
}

void loop() { eventLoop.update(); }
```

Sketch'ler 1.1'deki tabloyla birebir örtüşüyor. Sketch'lerde mesaj tipi, kimlik ya da süre yok.

### 3.5 Akış örneği: 3. daireye zil

```
Zil paneli (dış1)                 Kapı ünitesi (dış2)                    1. daire      2. daire      3. daire
─────────────────                 ───────────────────                    ────────      ────────      ────────
Buton 3 bırakıldı (50 ms–30 sn,
bekleme süresi dolmuş)
→ ButtonGroup
→ panelLink.requestRing(3)
→ SecureChannel.seal (bağlantı
  şifresi): kaynak panel, hedef 3
→ BurstSender: 3 kopya ──────────→ PanelLink: denetle, doğrula
                                  kaynak panel, tip zil, hedef daire mi
                                  sayacı NVS'ye yaz
                                  → intercom.ringFlat(3)
                                  → FloodRouter.send(3, kRingBell)
                                  → SecureChannel.seal (bina ağı
                                    şifresi): kendi MAC'i, kendi sayacı
                                  → BurstSender: 3 kopya ──────────────→ doğrula
                                                                         hedef ≠ ben → aktar ──→ …
                                                                                                 …aktar ─────→ doğrula
                                                                                                               hedef = ben
                                                                                                               sayacı NVS'ye yaz
                                                                                                               intercom.onRing
                                                                                                               bell.ring() 1,5 sn
```

### 3.6 Çerçeve biçimi (v2)

İki ağ da aynı biçimi kullanıyor. Ağları başlıktaki `networkId` ve şifre ayırıyor.

| Alan | Bayt | Koruma |
|---|---|---|
| `version` | 1 | imzalı |
| `networkId` | 4 | imzalı |
| `sourceMac` | 6 | imzalı |
| `source` | 1 | imzalı |
| `destination` | 1 | imzalı |
| `counter` | 4 | imzalı |
| `type` | 1 | şifreli + imzalı |
| `tag` | 8 | — |
| **Toplam** | **26** | ESP-NOW sınırı 250 bayt |

- **Nonce (13 bayt):** `sourceMac(6) | counter(4) | version(1) | 0(2)`. Tekilliğini fabrikadan tekil MAC (Karar 4) ve kartın tek, kalıcı sayacı (Karar 13) birlikte garanti ediyor. Ağ kimliği nonce'ta yok, çünkü her ağın anahtarı ayrı.
- **Bayt sırası:** Little-endian. Alanlar tek tek yazılır, struct'lar bellekten doğrudan kopyalanmaz.
- **Zil paneli bağlantısında** `source` her zaman `kBellPanelId`, `destination` çalınacak daire, `type` her zaman zil. Çerçeveyi alan kapı ünitesi (Karar 19).

**Gelen çerçevenin işlenme sırası:** Ucuz denetimler önce, kripto sonra, durum değişikliği en son.
1. Uzunluk 26 bayt mı? Değilse radyo çerçeveyi kuyruğa koymaz (`EspNowRadio`).
2. Radyo çerçeveyi karttaki her ağa verir (Karar 20). Her ağın `SecureChannel`'ında: sürüm ve `networkId` bu ağın mı? Değilse at (öteki ağ ya da komşu apartman).
3. `sourceMac` benim mi (kendi yankım) ya da `source`/`destination` olmayan bir ünite mi? Öyleyse at.
4. Gönderen MAC'in tekrar penceresine salt okunur bak. Görülmüş kopyayı şifre çözmeden at (Karar 16).
5. AES-CCM ile doğrula ve şifreyi çöz. Geçmezse at.
6. Gönderen MAC için pencere yoksa şimdi aç ve kayıtlı sayacı NVS'den yükle. Yer sadece doğrulanmış göndericiye ayrılıyor, sahte MAC'ler tabloyu dolduramıyor. Pencereyi ilerlet.
7. Mesaj tipi tanımlı mı?
8. **Bina ağı (`FloodRouter`):** Kaynak zil paneliyse at. Tip ile hedef uyuşuyor mu (herkese giden tip sadece herkese, diğerleri tek üniteye)? Hedef herkesse (heartbeat) aktar ve teslim et, kalıcı kayıt yok (Karar 18). Hedef bensem sayacı NVS'ye yaz, yazılamazsa at, sonra eylemi çalıştır. Değilsem çerçeveyi olduğu gibi aktar.
9. **Zil paneli bağlantısı (`PanelLink`):** Kaynak zil paneli, tip zil ve hedef geçerli bir daire mi? Değilse at. Sayacı NVS'ye yaz, yazılamazsa at, sonra zil isteğini işleyiciye ver. Aktarma yok.

### 3.7 Eşzamanlılık ve zaman

- **Tek iş parçacığı:** Bütün mantık Arduino'nun loop görevinde çalışır.
- **Alma:** ESP-NOW'ın alma geri çağrısı (Wi-Fi görevinde çalışır) sadece 26 baytlık çerçeveyi kopyalar, 8 çerçevelik statik bir FreeRTOS kuyruğuna koyar ve loop görevini bildirimle uyandırır. Üretici–tüketici kalıbı: paylaşılan durum yok, kilit yok. Kuyruğu loop görevinde `EspNowRadio::update()` boşaltır ve her çerçeveyi bağlı ağlara verir.
- **Döngü:** `EventLoop` her turda bütün bileşenleri günceller. Sonra en yakın `nextDeadlineMs()` zamanına ya da bir bildirime kadar bloklanır, en fazla 1 sn. İşlemci bu sürede WFI ile bekler.

---

## 4. Veriye dayalı sınırlar

| Parametre | Değer | Dosya | Dayanak |
|---|---|---|---|
| En kısa basış | 50 ms | input_config.h | Sıçrama < 10 ms (Ganssle) · EFT patlaması 15 ms (IEC 61000-4-4) · insan basışı ≈ 80–110 ms |
| Örnekleme periyodu | 5 ms | input_config.h | Ganssle: 1–5 ms |
| En uzun basış | 30 sn | input_config.h | HMI "basılı tut" zaman aşımı pratiği ≥ 30 sn. Tahmin, sahada gözden geçirilebilir. |
| Kapı darbesi | 1,5 sn (1–2 sn) | door_unit.h | Gereksinim |
| Zil darbesi | 1,5 sn (1–2 sn) | indoor.h | Gereksinim |
| Zil bekleme süresi | 3 sn | bell_panel.h | 1,5 sn darbe + 1,5 sn sessizlik |
| Kapı isteği bekleme süresi | 2 sn | indoor.h | Mühendislik tercihi |
| Eşleştirme basışı | 10–15 sn, bırakınca | door_unit.h | Kazara basış ve sıkışmış buton ayrılsın |
| Eşleştirme penceresi | 180 sn | door_unit.h | Zigbee BDB `bdbcMinCommissioningTime` |
| Heartbeat aralığı | 1 sn | link_config.h | Kopukluk birkaç saniyede fark edilsin, kanal yükü ~%5 (8 ünite) |
| Bağlantı LED darbesi | 10 ms | indoor.h | Her heartbeat'te kısa bir yanıp sönme. Heartbeat aralığından kısa olmalı |
| Tekrar sayısı | 3 kopya (1–5) | radio_config.h | Onaysız yayında 4 katta kaçırma ≈ %0,4 (Karar 10) |
| Tekrar aralığı | 20 ms | radio_config.h | Kısa bir parazit iki kopyayı birden bozmasın |
| Aktarma gecikmesi (jitter) | 0–10 ms rastgele | radio_config.h | Aktarıcılar arasında çakışmayı azaltır |
| Tekrar penceresi | 64 | replay_window.h | RFC 6347 / RFC 4303 varsayılanı |
| Sayaç rezervi | 1000 | tx_counter.h | OpenThread `STORE_FRAME_COUNTER_AHEAD` varsayılanı |
| Şifreleme | AES-128-CCM, 8 bayt etiket | ccm_cipher.h | 802.15.4 / Zigbee / Thread / BLE standardı |
| CPU | 80 MHz | power_config.h | Wi-Fi'ın çalıştığı en düşük frekans |
| TX gücü | 8 dBm (başlangıç), Long Range açık | radio_config.h | Super Mini anten raporları. Menzil yetmezse artırılır. |
| Watchdog | 5 sn, bekleme ≤ 1 sn | event_loop.h | sdkconfig |

Sınırlar ayar dosyalarında `static_assert` ile denetleniyor. Örnekler: kopya sayısı 1–5, kanal 1–13, en kısa basış < en uzun basış, aynı pine iki eleman bağlanamaz, pinler strapping/USB/UART pinine denk gelemez, daire butonu olmayan bir daireye bağlanamaz, ağ kimliği ve anahtarı sıfır olamaz, bina ağı ile zil paneli bağlantısının kimliği ve anahtarı aynı olamaz. Yanlış bir değer girildiğinde program derlenmez.

---

## 5. Konvansiyonlar

Kodlama kurallarının tam ve güncel listesi proje kökündeki `CLAUDE.md` dosyasında. Bu bölüm kuralların gerekçesini anlatır.

**Yazım kuralları:**
- Tanımlayıcılar İngilizce, açıklamalar Türkçe ve sadece satır sonunda.
- Sabitler: `kPascalCase`, namespace içinde `inline constexpr`. `#define` kullanılmıyor.
- Tipler PascalCase, fonksiyon ve değişkenler camelCase, private üyeler sonda `_` ile.
- Dosya ve klasör adları snake_case (sketch klasörü ile `.ino` adı aynı), başlık koruması `#pragma once`, enum'lar `enum class`.
- Bütün kod `yenizil` isim alanında. `using namespace yenizil;` sadece `.ino` dosyasında (kaynak dosya), başlık dosyalarında yok.
- Standart C++ başlıkları kullanılır: `<stdint.h>` yerine `<cstdint>`.
- Çerçeve ve anahtar düzenindeki sayılar adlandırılmış sabittir, konumlar `static_assert` ile birbirine bağlıdır.

**Sorumluluk ayrımı:**
- `.ino`: sadece bağlamalar.
- `hardware.h`: sadece dışarıdan bağlanan elemanlar.
- Ayarlar header dosyalarında. Yüklemeden önce değişenler `*_config.h` (ünite klasöründe), ünite ayarları `bell_panel.h` / `door_unit.h` / `indoor.h`, bina ve ürün ayarları `common/config/*_config.h` içinde.
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
| Güçlü tip (`enum class X : uint8_t {}`, `std::byte` kalıbı) | `NodeId`, `NetworkId`, `FrameCounter` | Kimlik, pin ve sayaçlar birbirine karışırsa derlenmez |
| `std::array` | Tekrar pencereleri, MAC adresi, çerçeve | Sabit boyut, heap yok |
| `std::optional` | `SecureChannel::seal/open()`, `TxCounter::next()`, `loadRxCounter()` | "Sonuç yok" durumunu tipin kendisi ifade eder |
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
| Wi-Fi modu, Long Range, sürekli dinleme, kanal, TX gücü, MAC adresi | `WiFi` (`enableLongRange`, `mode`, `setSleep`, `setChannel`, `setTxPower`, `macAddress`) |
| ESP-NOW başlatma, yayın eşi ve Long Range hızı, alma | `ESP_NOW` (`ESP_NOW.begin`, `ESP_NOW_Peer`, `onNewPeer`) |
| Kalıcı sayaçlar | `Preferences` (NVS) |
| Rastgele aktarma gecikmesi | `random()` (Wi-Fi açıkken donanım RNG) |
| Pin okuma/yazma | `pinMode`, `digitalRead`, `digitalWrite` |
| CPU frekansı | `setCpuFrequencyMhz()` |
| Watchdog | `enableLoopWDT()` |

**Arduino karşılığı olmadığı için ESP-IDF / FreeRTOS:**

| İş | API | Neden |
|---|---|---|
| 64-bit zaman | `esp_timer_get_time` | Arduino `millis()` 32-bit, 49,7 günde taşar (Karar 8) |
| Çıkışı açmadan önce pasif seviye yazmak | `gpio_set_level` | Arduino 3.x'te `digitalWrite()` `pinMode()`'dan önce çalışmıyor, açılışta röle titreyebilirdi |
| AES-128-CCM | mbedTLS `mbedtls_ccm_*` | Arduino core'da AES-CCM sarmalayıcısı yok. mbedTLS core'la birlikte geliyor, C3'te donanım AES kullanıyor. |
| Olay bekleme, alma kuyruğu | FreeRTOS task notification, statik kuyruk | Arduino'da görevler arası kuyruk ve bildirim API'si yok |

**Elle yazılanlar:**

| Kod | Neden hazır değil |
|---|---|
| `PressDetector` | Bounce2, OneButton gibi kütüphaneler en kısa/en uzun basış, bekleme süresi ve açılışta basılı butonu yok sayma kurallarını birlikte sağlamıyor. Kural katmanı yine yazılacağı için ek bağımlılık bir şey kazandırmıyor. |
| `ReplayWindow` | Arduino ve ESP-IDF'te bağımsız kullanılabilir bir tekrar penceresi yok. mbedTLS'inki DTLS oturumunun içinde. |
| Burst, flooding ve ağlara dağıtım (`EspNowRadio`, `BurstSender`, `FloodRouter`, `PanelLink`) | Arduino core'da flooding yapan ya da tek radyoda birden çok ağı ayıran bir kütüphane yok. Espressif'in aktarma destekli `esp-now` bileşeni ESP-IDF'e geçiş gerektirir. |
| `EventLoop` / `Component` | "En yakın zamana ya da ESP-NOW bildirimine kadar bekle" davranışını FreeRTOS sağlıyor. Üstündeki katman ince bir liste. |
| `PulseOutput` | Arduino `Ticker` geri çağrıyı başka bir görevde çalıştırıyor. Paylaşılan durum ve kilit gerekirdi. Zamanlamayı olay döngüsü zaten yapıyor. |
| `frame`, `protocol` | Uygulamaya özgü çerçeve biçimi |
| `TxCounter` | Rezervli sayaç (OpenThread yöntemi) Arduino ve ESP-IDF'te bağımsız bir bileşen olarak yok |
| Bayt sırası, arama, denetimler | C++ standart kütüphanesiyle (`std::bit_cast`, `std::find`, `<algorithm>`) |

---

## 6. Durum

**Kod (v2):** Tüm işlevler yazıldı: G/Ç, ağ ve aktarma, güvenlik, güç tasarrufu, denetimler. Kimlik ve şifreler config dosyalarından geliyor.

**v3 tasarımı (bölüm 8, Karar 21–28):** Karar verildi, kod bekliyor. v3 ile config dosyaları kalkacak, kurulum aşağıdaki maddelerin yerine eşleştirmeyle yapılacak (8.5–8.7).

**Kurulum için gerekenler:**
1. Bina ağının kimliği ve şifresi `door_unit_config.h` ile `indoor_config.h` içinde aynı olmalı. Zil paneli bağlantısının kimliği ve şifresi `bell_panel_config.h` ile `door_unit_config.h` içinde aynı olmalı. Değiştirilirse eşleşen dosyalara aynen yazılır.
2. `bell_panel` zil paneline (dış1) yüklenir. Butonların hangi daireyi çaldıracağı `bell_panel/hardware.h` içinde.
3. `door_unit` kapı ünitesine (dış2) yüklenir.
4. Her iç ünite için `indoor_config.h` içindeki `kFlatId` o dairenin numarasına ayarlanıp `indoor` yüklenir.
5. Kart: ESP32C3 Dev Module. "Erase All Flash Before Sketch Upload" kapalı (Karar 13). Kod seri port kullanmıyor.

## 7. Senin kararını bekleyenler (donanım)

**Kararlaşanlar:**
- Zil 3.3V ile çalışıyor, en fazla 2,5 mA çekiyor ve iç ünitenin zil pininden (GPIO0) doğrudan besleniyor. GPIO pininin güvenli sınırı ~20 mA olduğu için sürücü devre gerekmiyor. Zil ileride daha güçlü bir modelle (> 20 mA ya da bobinli) değiştirilirse araya transistör/MOSFET ve flyback diyot konmalı.
- Kapı rölesi 3.3V ile tetikleniyor ve kendi izole güç beslemesi var. Kapı ünitesinin ESP'si sadece tetik girişini sürüyor.
- Kendi bağlanan her LED'e 240 Ω seri direnç şart: bağlantı LED'i, röle ya da zil yerine takılan test LED'i. 3.3V'ta akım renge göre ~1–5 mA olur. Pin-GND arasındaki 10k pull-down akımı sınırlamaz. Dirençsiz LED pinden aşırı akım çeker, çipi ısıtır ve pini bozabilir (yük testinde eski dış ünitede yaşandı).
- Kapı ünitesi (dış2) ve kapı rölesi tetiği bina içinde. Bina dışında sadece zil paneli (dış1) var (Karar 2). Panelde bina ağının şifresi olmadığı için (Karar 19) flash şifrelemeye ve ESP-IDF'e geçişe gerek yok.
- Her ünite (9 kart) 5V 300 mA adaptörle besleniyor. Ortalama tüketim ~85–100 mA (radyo sürekli dinliyor). Gönderim anında tepe akım 8 dBm'de tahminen 150–200 mA. Anlık düşüşlere karşı kartın 5V ve GND uçları arasına 470 µF elektrolitik kondansatör önerilir. TX gücü 14 dBm'in üstüne çıkarılacaksa en az 500 mA'lik adaptör gerekir.

**Açık kalan:**
1. Zil panelinin buton hatları için koruma: her butonun pini ile kablosu arasına 1 kΩ seri direnç, pin ile GND arasına 100 nF kondansatör. Butona dokunan elden gelen statik elektriği ve kablonun topladığı paraziti azaltır. Direnç pine giden akımı sınırlar, kondansatör kısa sıçramaları yutar. Yazılım 50 ms'den kısa basışları zaten yok sayıyor, bu donanım önlemi daha çok pini korumak için.
2. Panel aydınlatma LED'i (dış1 GPIO10) ve daire durum LED'leri (dış2): davranışları 8.4'te belirlendi, kod v3 ile gelecek. Şimdilik kod bu pinlere dokunmuyor, LED'ler sönük. Aydınlatma LED'i zil onayında yanıp söndüğü için GPIO'da kalmalı. Tek LED + 240 Ω ise doğrudan pine bağlanır. Birden çok LED ya da şerit olacaksa ~20 mA'i aşar, araya MOSFET gerekir.
3. Zil panelinin kutusu: bina dışında olduğu için yağmura, neme ve güneşe dayanıklı (ör. IP65) olmalı, adaptörü de buna göre korunmalı. Kart ile kapı ünitesi arasındaki menzil sahada denenmeli. Duvar ya da metal kapı sinyali zayıflatırsa kapı ünitesi kapıya yakın konur ya da TX gücü artırılır.

## 8. v3 tasarımı: eşleştirme (karar verildi, kod bekliyor)

### 8.1 Hedefler

- Bütün iç ünitelerde ve dış1'lerde aynı yazılım. Config dosyası yok, kartlar kurulumda kendilerini yapılandırıyor.
- Binaya özgü tek kart dış2. A ve B apartmanlarının ağları asla karışmıyor.
- Bir iç ünite sökülüp başka bir binada başka bir daireye eşleştirilebiliyor. Arızalı cihaz değiştirilebiliyor, sakin sonradan kendi cihazını alıp katılabiliyor.
- Eşleştirme apartman girişinde, iç ünite dış2'nin yanındayken yapılıyor. Yetki vidalı kutuyu açandayken.

### 8.2 Kayıtlar (NVS)

| Kart | Kayıt | Nereden |
|---|---|---|
| Dış2 | Bina ağının kimliği ve şifresi, bağlantının kimliği, radyo kanalı | İlk açılışta kendisi üretir (Karar 22, 28) |
| Dış2 | Kanal tablosu: daire 1–7 → `{MAC, cihaz anahtarı, kalıcı mı}` | Eşleşme |
| Dış2 | Dış1 kaydı: `{MAC, cihaz anahtarı}` | Eşleşme |
| Dış2 | MAC başına görülen en yüksek sayaç (sayaç tabanı) | Çalışırken |
| İç ünite | Ağ kimliği ve şifresi, radyo kanalı, daire numarası, cihaz anahtarı, dış2'nin MAC'i | Eşleşme |
| Dış1 | Bağlantının kimliği, radyo kanalı, cihaz anahtarı, dış2'nin MAC'i | Eşleşme |
| Hepsi | Gönderme sayacı rezervi, gönderen MAC başına son eylem sayacı | Bugünkü gibi (Karar 3, 13) |

Eşleşmemiş bir iç ünite ya da dış1 hiçbir şey göndermez ve aktarma yapmaz. Dış2 tablosunda olmayan bir MAC'ten gelen kapı açma, "buradayım" ve "çaldım" mesajlarını atar.

### 8.3 Butonlar

| Kart | Buton | 50 ms – 5 sn | 10–15 sn |
|---|---|---|---|
| İç ünite | Kapı aç (GPIO10) | Normal modda: kapıyı aç<br>Eşleşme modunda: sonraki kanal | Normal modda: eşleşmeye gir<br>Eşleşme modunda: onayla |
| Dış2 | BOOT (GPIO9, kart üstünde) | Eşleşme modunda: seçili dolu kanalı sil | Eşleşme modunu aç |
| Dış1 | BOOT (GPIO9, kart üstünde) | — | Eşleşme modunu aç |
| Dış1 | Zil butonları | Zil | — |

- 5–10 sn arası ve 15 sn'den uzun basışlar yok sayılıyor. Bugünkü 30 sn'lik üst sınır 5 sn'ye iniyor, yoksa uzun basış kapıyı açar ya da zili çalar.
- Kanal seçilmeden yapılan 10–15 sn basış onay sayılmıyor.
- Eşleşme modu 30 sn işlem yapılmazsa kapanıyor (Karar 21).
- Kart üstündeki BOOT butonu `hardware.h`'ye değil, kart tanımına (`board_pins.h`) girer.

### 8.4 LED'ler

**Dış2 daire LED'leri:**

| Kanal durumu | Normal mod | Eşleşme modu |
|---|---|---|
| Boş | sönük | yanıp söner (500/500 ms) |
| Dolu, cihaz çevrimiçi | sürekli yanık | sönük |
| Dolu, cihaz çevrimdışı (30 sn sessiz) | sönük (arıza işareti) | sönük |
| İç ünitenin şu an seçtiği kanal | — | sürekli yanık |
| Hata (ikinci aday, red, süre doldu) | — | bütün LED'ler 3 sn hızlı yanıp söner (100/100 ms) |
| Dış1 eşleşti | — | bütün LED'ler iki kez yanıp söner |

**İç ünite bağlantı LED'i ve zili:**

| Durum | Gösterge |
|---|---|
| Eşleşmiş, normal | Dış2'nin her heartbeat'inde 10 ms yanar (bugünkü gibi) |
| Eşleşmemiş | sönük |
| Eşleşme modunda | hızlı yanıp söner (100/100 ms) |
| Eşleşme başarılı | 2 sn sürekli yanar |
| Eşleşme reddedildi ya da süre doldu | zil 3 kez kısa çalar |

**Dış1 aydınlatma LED'i:** Sürekli yanık (saat kaynağı yok, 1.3). Zil onayı gelirse 5 kez kısa yanıp söner (150/150 ms), 2 sn içinde gelmezse 1 sn sönüp geri yanar (Karar 26).

### 8.5 İç ünite eşleşme akışı

```
İç ünite                                            Dış2
────────                                            ────
                                                    BOOT 10–15 sn → eşleşme modu
                                                    boş kanallar yanıp söner, dolular sönük
Kapı butonu 10–15 sn → eşleşme modu
JoinHello {açık anahtar} ── kanal 1, 6, 11 sırayla ──→ tek aday mı? sinyal güçlü mü?
                         ←── JoinOffer {açık anahtar} ── ikisi de X25519 + HKDF:
                                                         oturum anahtarı + cihaz anahtarı
Kısa basış ── SelectNext ───────────────────────────→ ilk: en küçük boş kanal, sonra 1→7 döngüsel
                         ←── Selected {kanal} ───────── seçili kanalın LED'i sürekli yanar
(kanal doluysa)                                     BOOT kısa basış → kanal silinir
10–15 sn ── Confirm {kanal} ────────────────────────→ kanal boş mu? geçici kayıt
                         ←── Welcome {ağ kimliği, ── (oturum anahtarıyla şifreli)
                              ağ şifresi, radyo kanalı,
                              daire no, sayaç tabanı}
NVS'ye yaz, eşleşme modundan çık
"Buradayım" (cihaz anahtarıyla) ───────────────────→ kayıt kalıcı olur
```

**Kurallar:**
- **Tek oturum:** Dış2 aynı anda tek adayla oturum açıyor. Oturum bitince pencere açık kalıyor ve süre baştan başlıyor. Sıradaki cihaz hemen eşleşebiliyor.
- **Güvenilir istek–cevap:** İç ünite cevap gelene kadar isteği tekrarlıyor, dış2 aynı isteğe aynı cevabı veriyor.
- **Geçici kayıt:** Cihaz anahtarıyla imzalı ilk "buradayım" 60 sn içinde gelmezse siliniyor. Yarım kalan eşleşme hayalet kayıt bırakmıyor.
- **Sayaç tabanı:** Flash'ı silinip aynı binaya dönen cihazın sayacı 0'dan başlar, ve bu cihaz tekrar sanılırdı. Bunu önlemek için iç ünite sayacına dış2'nin o MAC için gördüğü en yüksek değerden devam ediyor.
- **Bir MAC, bir kanal:** Aynı binada başka kanala geçen cihazın eski kaydı otomatik siliniyor.
- **Eski eşleşme korunuyor:** Eşleşmiş bir cihaz eşleşme moduna girip onay vermeden çıkarsa eski eşleşmesi bozulmuyor. Yeni bir onay eski ağ bilgilerinin üstüne yazıyor, taşımak için fabrika ayarına dönmek gerekmiyor.

### 8.6 Dış1 eşleşmesi

Dış2 eşleşme modundayken dış1'in BOOT butonuna 10–15 sn basılıyor. Aynı JoinHello / JoinOffer akışı çalışıyor, ama kanal seçimi yok. Welcome bağlantının kimliğini, radyo kanalını ve sayaç tabanını taşıyor, cihaz anahtarı bağlantının şifresi oluyor. Yeni bir dış1 eşleşirse eskisinin kaydının yerine geçiyor.

### 8.7 Senaryolar

| Durum | Akış |
|---|---|
| İlk kurulum | Dış2 eşleşme modunda, iç üniteler sırayla yanına getirilip eşleştirilir. Ardından dış1 eşleştirilir. |
| Sakin kendi cihazını alır | Kanal boşsa 8.5'teki akış |
| Bozulan cihaz değişir | Yeni cihaz dolu kanala gelir (sönük LED seçilince yanar) → dış2'de BOOT kısa basış ile kanal silinir → iç ünitede onay. Eski cihaz kapıyı açamaz (Karar 24). |
| A5 → B3 taşıma | B'de normal eşleşme yapılır. A'da 5. kanal kendiliğinden boşalmaz. Herhangi bir iç ünite "kumanda" olarak eşleşme moduna sokulur, 5. kanal seçilir, BOOT kısa basış ile silinir, onay vermeden beklenir. Kumandanın kendi eşleşmesi bozulmaz. |
| Dış2 bozulur | Yeni dış2 yeni bir kimlikle başlar. Bütün iç üniteler ve dış1 yeniden eşleşir. |

### 8.8 Normal çalışma

| Mesaj | Yön | Ağ anahtarı | Cihaz anahtarı | Ne zaman |
|---|---|---|---|---|
| Heartbeat | Dış2 → herkes | ✓ | — | 1 sn'de bir (Karar 18) |
| Buradayım | Daire → dış2 | ✓ | ✓ | 10 sn'de bir (Karar 25) |
| Kapıyı aç | Daire → dış2 | ✓ | ✓ | Kısa basış |
| Zil çal | Dış2 → daire | ✓ | — | Panelden istek gelince |
| Çaldım | Daire → dış2 | ✓ | ✓ | Zil tetiklenince |
| Zil isteği | Dış1 → dış2 (bağlantı) | — | ✓ (bağlantı anahtarı) | Zil butonu |
| Zil onaylandı {daire} | Dış2 → dış1 (bağlantı) | — | ✓ (bağlantı anahtarı) | "Çaldım" gelince |

### 8.9 Çerçeveler

| Çerçeve | Boyut | İçerik |
|---|---|---|
| Bina / bağlantı, tek etiket | 27 bayt | Başlık 17 + yük 2 (tip + argüman) + etiket 8 |
| Bina, iki etiket | 35 bayt | Yukarıdaki + cihaz anahtarıyla iç etiket 8 (yükle birlikte şifreli) |
| Eşleşme (JoinHello, JoinOffer) | ~50 bayt | Sürüm, rol, MAC, 32 bayt açık anahtar. Şifresiz, aktarılmaz. |
| Eşleşme (SelectNext … Welcome) | ≤ 64 bayt | Oturum anahtarıyla AES-CCM, aktarılmaz |

- Yükteki argüman baytı daire numarasını taşıyor ("zil onaylandı"). Çerçeve biçimi değiştiği için protokol v3 oluyor, bütün kartlar yeniden yükleniyor.
- Radyo bugün sadece 26 baytlık çerçeveyi kabul ediyor. Bilinen boyutları kabul edip türe göre dağıtacak.
- Nonce bugünkü gibi `MAC + sayaç`. Anahtarlar farklı olduğu için iki etiketin aynı sayacı kullanması güvenli.

### 8.10 Durum makineleri

**İç ünite:**
```
Eşleşmemiş ──10–15 sn──▶ Arıyor ──JoinOffer──▶ Seçiyor ──10–15 sn (kanal seçili)──▶ Onay bekliyor ──Welcome──▶ Eşleşmiş
                           │                      │                                     │
                           └──── 30 sn / ikinci cevap / red ──────────────────────────┴──▶ önceki durum
Eşleşmiş ──10–15 sn──▶ Arıyor   (eski bilgiler yeni onaya kadar korunur)
```

**Dış2:**
```
Normal ──BOOT 10–15 sn──▶ Pencere açık ──JoinHello (tek aday, güçlü sinyal)──▶ Oturum(MAC) ──Confirm──▶ geçici kayıt, Welcome
                             ▲   │                                               │
                             │   └── 30 sn işlem yok ──▶ Normal                   ├── ikinci aday ──▶ Hata (3 sn) ──▶ Normal
                             └──────────── oturum bitti, süre baştan ─────────────┘
```

**Dış1:**
```
Eşleşmemiş ──BOOT 10–15 sn──▶ Arıyor ──Welcome──▶ Eşleşmiş
Eşleşmiş: zil butonu ──▶ Onay bekliyor (2 sn) ──onay──▶ 5 kez yanıp söner
                                               └─süre doldu──▶ 1 sn söner
```

### 8.11 v3 ile değişecek mevcut kararlar ve kurallar

| Karar / kural | Değişiklik |
|---|---|
| Karar 4 | Daire numarası eşleşmede veriliyor (kanal = daire). Nonce MAC'ten üretilmeye devam ediyor. |
| Karar 10 | Aktarıcı kopya sayısı Karar 27'ye göre |
| Karar 11 | Config dosyaları kalkıyor (Karar 22) |
| Karar 13 | "Flash'ı asla silme" kuralı yumuşuyor: silinen kart şifrelerini de kaybeder ve yeniden eşleşir, nonce tekrarı olmaz. Dış2 silinirse bina yeniden kurulur. |
| Karar 19 | Bağlantının kimliği ve şifresi eşleşmede oluşuyor |
| 4. bölüm | Kapı basışı ve zil basışı üst sınırı 30 sn → 5 sn, eşleşme 10–15 sn, pencere 30 sn |
| `CLAUDE.md` | Kurulum istisnası ("daire numarası config'de"), config dosyaları ve güvenlik kuralları v3 koduyla birlikte güncellenecek |

---

## Kaynaklar

- ESP32-C3 datasheet (güç tüketimi): https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf
- Kurulu core yapılandırması: `%LOCALAPPDATA%/Arduino15/packages/esp32/tools/esp32c3-libs/3.3.12/sdkconfig`
- Ganssle, A Guide to Debouncing: https://www.ganssle.com/debouncing.htm · https://www.ganssle.com/debouncing-pt2.htm
- IEC 61000-4-4 EFT/Burst: https://en.wikipedia.org/wiki/IEC_61000-4-4
- Tuş basılı tutma süreleri: https://wraitor.io/learn/keystroke-dynamics
- RFC 6347 (DTLS 1.2) tekrar penceresi: https://www.rfc-editor.org/rfc/rfc6347
- OpenThread `STORE_FRAME_COUNTER_AHEAD`: https://openthread.io/reference/config/group/config-misc
- RFC 6206 (Trickle): https://www.rfc-editor.org/rfc/rfc6206
- RFC 7731 (MPL): https://www.rfc-editor.org/rfc/rfc7731
- RFC 7748 (X25519): https://www.rfc-editor.org/rfc/rfc7748
- RFC 5869 (HKDF): https://www.rfc-editor.org/rfc/rfc5869
- HMI buton zaman aşımı pratiği: https://industrialmonitordirect.com/blogs/knowledgebase/hmi-button-set-while-pressed-timeout-issues-and-best-practices
