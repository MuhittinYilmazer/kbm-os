# 10 — Limine bellek haritası ve sıradaki bellek katmanı

Bu bölüm, `main.c` sonunda yazdırılan ham bellek satırlarının ne söylediğini ve henüz yazılmamış bellek yöneticisinin hangi problemi çözeceğini anlatır.

## 1. İki farklı “harita”

Önce birbirine çok benzeyen iki kavramı ayıralım.

- **Fiziksel bellek haritası (memory map)**: Makinedeki fiziksel adres aralıklarının ne amaçla ayrıldığını söyler. Örneğin “bu aralık kullanılabilir RAM”, “bu aralık bootloader tarafından tutuluyor” ya da “bu aralık framebuffer”.
- **Sayfa eşlemesi (page mapping)**: CPU'nun bir sanal adresi hangi fiziksel adrese çevirebildiğini söyler.

Birincisini Limine verir; ikincisini CPU, CR3 ve sayfa tabloları belirler. Bir fiziksel aralığın “kullanılabilir” olması, onun otomatik olarak her sanal adresten erişilebilir olduğu anlamına gelmez. Tersine, bir sanal adres eşlenmiş olabilir ama karşılığı RAM değil bir MMIO aygıt kaydı olabilir. LAPIC bölümü bunun canlı örneğiydi.

## 2. Limine'den gelen bellek haritası

`main.c`, `limine_memmap_request` ile Limine'den bellek haritası ister. Cevapta iki önemli alan vardır:

~~~c
memmap_request.response->entry_count
memmap_request.response->entries[i]
~~~

Her giriş kabaca şu üç bilgiyi taşır:

| Alan | Anlamı |
| --- | --- |
| `base` | Fiziksel başlangıç adresi |
| `length` | Bu aralığın byte cinsinden uzunluğu |
| `type` | Aralığın kullanım türü |

Kod girişleri seri porta yazdırır ve PMM bu `USABLE` bilgiyi bitmapini kurmak
için kullanır. Önce kaynak hakkında güvenilir veri almak, sonra onu yönetmek
hâlâ doğru başlangıç sırasıdır.

QEMU'yu `-m 2G` ile açtığında büyük bir `USABLE` bölge görmen normaldir. Bunun yanında düşük adreslerde ayrılmış alanlar, kernel/bootloader için tutulmuş alanlar ve örneğin framebuffer için RAM-dışı özel alanlar da görülür. Görünen giriş sayısı ve kesin adresler QEMU sürümüne, RAM miktarına ve önyükleme biçimine göre değişebilir; sayıları ezberleme.

## 3. Türler: hangi bellek hemen kullanılabilir?

Limine türleri `limine.h` içinde tanımlıdır. Bu kernel için pratik yorum şöyledir:

| Tür | İlk allocator açısından anlamı |
| --- | --- |
| `USABLE` | Aday serbest fiziksel RAM. İlk fiziksel allocator burada çalışır. |
| `BOOTLOADER_RECLAIMABLE` | Bootloader işini bitirdikten sonra ileride geri alınabilir; başlangıçta temkinli davranılır. |
| `EXECUTABLE_AND_MODULES` | Kernel ve modüllerin bulunduğu alan; kendi kodunu ezmemelisin. |
| `FRAMEBUFFER` | Ekran belleği; normal genel amaçlı RAM gibi dağıtılmaz. |
| `RESERVED`, `ACPI_*`, `BAD_MEMORY` | Sahipliği veya anlamı özel alanlar; normal allocator bunları serbest saymaz. |

İlk güvenli politika basittir: yalnızca `USABLE` girişlerinden sayfa ver, diğer her şeyi dokunulmaz kabul et. Bu biraz bellek boşa harcayabilir ama yanlış adresi ezip makineyi bozma riskini azaltır.

## 4. “16 GB RAM neden birkaç kayıt?” sorusu

Bellek haritası RAM'i tek tek byte veya tek tek DIMM olarak listelemez. Aynı niteliğe sahip büyük bir **aralık** tek kayıt olur.

Örnek olarak 2 GiB'lık bir kullanılabilir aralık şöyle okunabilir:

~~~text
base   = 0x00100000
length = 0x7fe3d000
type   = USABLE
~~~

Bu “tek parça RAM çipi var” demek değildir. “Bu fiziksel adres aralığındaki byte'ların bu kadarını kernel genel amaçlı RAM olarak kullanabilir” demektir. Bir allocator daha sonra bu büyük aralığı 4 KiB'lık fiziksel **frame**'lere böler.

Bir **page**, sanal adres alanındaki 4 KiB'lık bir kutudur. Ona karşılık gelen fiziksel 4 KiB'lık kutuya çoğu zaman **frame** denir. İkisi aynı boyda olabilir ama biri adresleme görünümü, diğeri gerçek RAM kaynağıdır.

## 5. Sıradaki üç allocator ve görev ayrımı

“Bump allocator, bitmap allocator, heap allocator” üç ayrı isimdir; fakat aynı katmanda rakip üç araç değildir.

### 5.1 Fiziksel bump allocator

Bir kullanılabilir aralığın başını tutar. “Bir frame ver” dendiğinde sıradaki hizalı 4 KiB'ı verir ve işaretçiyi ileri alır.

~~~text
başlangıç -> [verildi][verildi][sıradaki][boş][boş]...
                                  ^
~~~

Avantajı: çok kısa, ilk sayfa tablolarını veya ilk veri yapılarını ayağa kaldırmak için idealdir.

Eksikliği: geri verme yoktur. Bir frame bırakıldığında tekrar kullanamaz. Bu nedenle kernelin nihai bellek yöneticisi değildir.

### 5.2 Fiziksel bitmap allocator

Her fiziksel frame için bir bit tutar:

~~~text
frame:  0 1 2 3 4 5 6 7
bit:    1 1 0 1 0 0 1 0
        ^ kullanılıyor     0 = boş, 1 = ayrılmış (seçtiğimiz kurala göre)
~~~

Böylece belirli bir frame'i ayırabilir ve sonra serbest bırakabilirsin. Gerçek bir bellek yöneticisi için daha uygundur. Zorluğu, bitmap'in kendisini güvenli bir RAM yerine koymak ve kernel/bootloader/MMIO alanlarını başlangıçta doğru işaretlemektir.

### 5.3 Kernel heap allocator

Heap, `kmalloc` benzeri çağrıların istediği 24 byte, 100 byte veya yapı boyutu kadar **küçük** bloklar içindir. Heap doğrudan fiziksel RAM'den rastgele byte vermez. Önce fiziksel allocator'dan frame ister, bu frameleri kernel sanal alanına map eder; sonra kendi içinde küçük bloklara böler.

~~~text
fiziksel allocator -> 4 KiB frame
sayfa eşleyici       -> kernel sanal adresinde erişilebilir sayfa
heap allocator       -> çağırana 24/100/... byte blok
~~~

Bu yüzden doğrudan heap ile başlamak teknik olarak mümkündür ama alttaki iki katmanın yaptığını gizlice yine çözmek zorunda kalırsın. Öğrenme sırası olarak bump → bitmap → heap mantıklıdır. Her biri bir sonrakinin dayandığı sorunu görünür yapar.

## 6. Bu projede henüz ne yok?

KBM artık fiziksel frame dağıtan ve serbest bırakan iki-bitmapli bir PMM ile
16-byte hizalı küçük bir bump heap'e sahiptir. Güncel uygulama, testler ve
sınırlar Bölüm 13'te anlatılır. HHDM altında RAM'e erişmek yine RAM'i
“sahiplenmek” değildir; allocator hangi frame'i kimin kullanabileceğini takip eder.

Bu ayrım kritik:

~~~text
HHDM:       “Bu fiziksel adrese erişebilirim.”
allocator:  “Bu fiziksel frame şu anda bana ait ve güvenle kullanılabilir.”
~~~

## 7. Uygulama planı

Bir sonraki bellek uygulamasını şu sırayla yap:

1. `USABLE` girişlerini tara ve 4 KiB hizalı uygun ilk aralığı seç.
2. Kernelin, Limine yapıların ve geçici kritik verilerin üzerine gelmemesine dikkat et.
3. Sadece frame veren küçük bir fiziksel bump allocator yaz.
4. Birkaç frame ayırıp seri porttan fiziksel adreslerini yazdır.
5. Ardından bitmap için kaç frame ve kaç bit gerektiğini hesapla.
6. Bitmap'i güvenli bir kullanılabilir aralığa yerleştir; kullanılmaması gereken frameleri işaretle.
7. Fiziksel allocator güvenilir hale gelmeden heap yazmaya çalışma.

İlk sürümün amacı “tüm RAM'i verimli kullanmak” değil, **yanlış belleği asla dağıtmamak** olmalı.

## Kontrol noktası

Bu bölümü bitirdiğinde şunları kendi cümlenle söyleyebilmelisin:

- Limine memory map, page table değildir.
- `USABLE` bir aday havuzdur; her entry hemen serbest değildir.
- HHDM erişim kolaylığıdır, allocator sahiplik takibidir.
- Bump allocator frame'i hızlı verir ama geri almaz.
- Bitmap fiziksel frameleri izler; heap daha küçük kernel tahsislerini yönetir.
