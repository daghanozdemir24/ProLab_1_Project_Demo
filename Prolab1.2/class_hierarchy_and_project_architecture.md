# GEZEGEN POSTACILARI - SINIF VE PAKET MİMARİSİ

Bu şema, projede istenen Nesneye Yönelik Programlama (OOP) prensiplerine, `instanceof` / `switch-case` yasağına ve Command Pattern mimarisine tam uyumlu paket ve sınıf hiyerarşisini gösterir. Hiçbir iş mantığı kodu içermez; sadece sınıfların görev alanlarını ve sorumluluklarını tanımlar.

---

## 1. Paket Yapısı (Package Structure)

```text
src/
└── tr/edu/kou/prolab/
    ├── config/           # Sabit değerler ve konfigürasyon
    ├── model/
    │   ├── common/       # Temel veri tipleri (Konum, Yön, Durum vb.)
    │   ├── capability/   # Yetenek arayüzleri (Polymorphism / Interface)
    │   ├── robot/        # Robot soyut sınıfı ve gövde çeşitleri
    │   └── ground/       # Zemin soyut sınıfı ve zemin çeşitleri
    ├── engine/           # Simülasyon döngüsü, ızgara (Grid) ve kurallar
    ├── command/          # Komut Deseni (Command Pattern) sınıfları
    ├── io/               # Dosya okuma (Parser) ve Hata yönetimi
    ├── ui/               # Görsel Arayüz (Swing / JavaFX)
    └── Main.java         # Program giriş noktası
```

---

## 2. Sınıf Listesi ve Sorumluluk Şeması

### A. Konfigürasyon ve Sabitler (`config`)
* `GameConstants`: Tüm enerji maliyetleri (Düz zemin, Toz fırtınası, Krater, Metan gölü, Buz kabuğu maliyetleri), varsayılan değerler ve sınırların tutulduğu merkezi sınıf. (Hoca maliyet değiştir dediğinde sadece burası değişir).

---

### B. Temel Tipler ve Ortak Modeller (`model.common`)
* `Position`: Robotların ve hücrelerin $(x, y)$ koordinatlarını tutar.
* `Direction`: Robotun yönünü tutan enum (`NORTH`, `EAST`, `SOUTH`, `WEST`). Sağa ve sola 90 derece dönüş mantıklarını barındırır.
* `RobotStatus`: Robotun anlık durumunu belirten enum (`AKTIF`, `TESLIM`, `ENERJISIZ`).

---

### C. Yetenek Arayüzleri (`model.capability`)
> **Kritik:** Şartnamedeki *"Zemin kuralları gövdenin sınıf adına değil yeteneğine bakmalıdır"* kuralını sağlayan arayüzler. `instanceof` yerine polimorfizm / double-dispatch sağlar.
* `Flyable`: Uçabilme/havada asılı kalma yeteneği (Metan gölünden geçebilir, toz fırtınasında savrulur).
* `Jumpable`: Engelin üzerinden atlayabilme yeteneği (Kraterli kayayı 1 hücre atlayarak geçer).
* `Slidable`: Buz üzerinde sürtünmesiz kayabilme yeteneği (Buz bitene kadar tek hamlede kayar).
* `Swimmable`: *(Özel gövde için)* Sıvı/lav/su üzerinde yüzebilme yeteneği.

---

### D. Robot ve Gövde Hiyerarşisi (`model.robot`)
* `Robot` *(abstract class)*: Konum, yön, enerji, id ve durum değişkenlerini kapsüller (`private`). Enerji düşürme, hareket etme, dönme işlemlerini kendi metotlarıyla yönetir.
* `TrackedRobot` (`PaletliRobot`): Standart yeteneklere sahip paletli gövde sınıfı.
* `JumpingRobot` (`SicrayanRobot`): `Robot` sınıfından türer, `Jumpable` arayüzünü uygular.
* `AirborneRobot` (`HavaliRobot`): `Robot` sınıfından türer, `Flyable` arayüzünü uygular.
* `SledRobot` (`KizakliRobot`): `Robot` sınıfından türer, `Slidable` arayüzünü uygular.
* `AmphibianRobot` (`YuzenRobot`): *(Gereksinimdeki Kendi Eklediğiniz Gövde)* `Robot` sınıfından türer, `Swimmable` uygular.

---

### E. Zemin Hiyerarşisi (`model.ground`)
* `Ground` *(abstract class)*: Her hücrenin zemin tipini temsil eder. Robotun bu zemine girişini yöneten soyut metot (`enter(Robot robot, Grid grid)`) barındırır.
* `FlatGround` (`DuzZemin`): 1 enerji harcatan standart zemin.
* `DustStormGround` (`TozFirtinasi`): Standart robotlardan 2 enerji alır; `Flyable` ise savurur ve durdurur.
* `CraterRockGround` (`KraterliKaya`): `Jumpable` ise arkadaki hücreye atlatır; değilse 3 enerji alır veya geçit vermez.
* `MethaneLakeGround` (`MetanGolu`): `Flyable` olmayan tüm robotları durdurur.
* `IceGround` (`BuzKabugu`): `Slidable` olanı zemin bitene kadar kaydırır; diğerlerinden standart maliyet alır.
* `ObstacleGround` (`Engel` - 'X'): Hiçbir robotun girmesine izin vermez.
* `DeliveryRampGround` (`TeslimRampasi` - 'H'): Paketi teslim alır, robotu `TESLIM` durumuna geçirir ve haritadan kaldırır.
* `LavaGround` (`LavGolu`): *(Gereksinimdeki Kendi Eklediğiniz Zemin)* Sadece `Swimmable` robotların geçebildiği özel zemin.

---

### F. Komut Mimarisi (`command`)
* `Command` *(interface)*: `execute(Robot robot, Grid grid)` metodunu tanımlayan Command Pattern arayüzü.
* `MoveForwardCommand` (`IleriCommand`): `n` adım ileri gitme komutu.
* `TurnRightCommand` (`SagCommand`): Robotu 90 derece sağa döndürür.
* `TurnLeftCommand` (`SolCommand`): Robotu 90 derece sola döndürür.

---

### G. Simülasyon ve Harita Izgarası (`engine`)
* `Grid` / `PlanetMap`: $g \times y$ boyutundaki iki boyutlu haritayı, zemin matrisini ve hücrelerdeki robot doluluklarını takip eden sınıf.
* `SimulationManager`: Haritayı, robot listesini ve komut kuyruğunu tutar. Sırasıyla (Robot 1 -> Robot 2 -> ...) komutları işleyen simülasyon motorudur.
* `SimulationState`: Bir komut sonrasındaki harita anlık görüntüsünü (snapshot) tutar (Arayüzde "Geri" / "İleri" butonları için Memento pattern benzeri durum saklama).

---

### H. Dosya Okuma, Doğrulama ve İstisnalar (`io`)
* `MapFileReader`: `harita.txt` dosyasını satır satır okur ve haritayı oluşturur.
* `CommandFileReader`: `komutlar.txt` dosyasını okuyup `Command` nesneleri listesine dönüştürür.
* `ConsoleOutputFormatter`: Kılavuzda istenen formatta referans metin çıktısını (`ROBOT 1 ... TESLIM 2 KALAN ENERJI 43`) konsola basar.
* `InvalidMapException`: Hatalı harita formatı, çift robot ID'si veya geçersiz zemin karakteri durumunda fırlatılır.
* `InvalidCommandException`: Bilinmeyen komut veya `ILERI 0` gibi geçersiz parametre durumunda satır numarasıyla fırlatılır.

---

### I. Kullanıcı Arayüzü (`ui`)
* `MainFrame`: Uygulama ana penceresi (Menüler, buton panelleri ve ızgarayı birleştiren yapı).
* `GridCanvas` / `BoardPanel`: Harita matrisini, zemin renklerini ve robotların anlık konumlarını çizen panel.
* `ControlPanel`: "Oynat", "İleri", "Geri", "Sonraki Komut" ve "Hız Ayarı" butonlarını barındıran kontrol paneli.
* `RobotStatusPanel`: Sağ tarafta her robotun kalan enerjisini, durumunu (`yolda`, `teslim etti`) gösteren panel.

---

## 3. İki Kişi İçin Sorumluluk Paylaşım Matrisi

| Paket / Alan | Sorumlu | Teslim Edeceği Çıktı |
| :--- | :---: | :--- |
| **model.robot**, **model.capability** | **1. Kişi** | Robot sınıfı, yetenek arayüzleri ve robot gövdeleri |
| **model.ground** | **1. Kişi** | Zemin sınıfları ve çift yönlü etkileşim kural metotları |
| **config**, **model.common** | **1. Kişi** | Sabitler, Konum, Yön ve Durum yapıları |
| **io** (Parser & Exceptions) | **2. Kişi** | `harita.txt` ve `komutlar.txt` okuyucu, özel exception'lar |
| **command** | **2. Kişi** | `Command` arayüzü ve İleri/Sağ/Sol komut sınıfları |
| **engine** | **Ortak** | `Grid` ve `SimulationManager` (Sınıf imzaları birlikte kararlaştırılır) |
| **ui** (GUI) | **2. Kişi** | Swing/JavaFX arayüzü, animasyon/adım adım ilerleme |
| **ConsoleOutputFormatter** | **2. Kişi** | Standart referans konsol çıktısının üretilmesi |
| **Özel Gövde / Zemin Tasarımı** | **1. Kişi** | Ekstra gövde ve zeminin eklenmesi, kural tanımı |
| **IEEE Raporu & UML Çizimi** | **2. Kişi (Ağırlıklı)** | Şemaya uygun UML diyagramı ve rapor taslağı |