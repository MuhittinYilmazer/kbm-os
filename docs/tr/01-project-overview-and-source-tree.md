# Proje özeti ve kaynak ağacı

KBM, Limine tarafından yüklenen, x86-64 hedefli bir freestanding kerneldir.
Freestanding, C programının Linux, glibc veya normal C çalışma zamanına
dayanmadığı anlamına gelir. Kernel kendi başlangıç koduna, bellek rutinlerine ve
donanım erişimine sahip olmalıdır.

## Mevcut çalışma özeti

KBM bugün şunları yapar:

~~~
Limine ile boot olur.
COM1 seri porta log yazar.
Temel framebuffer çizim fonksiyonları içerir.
Kendi GDT ve IDT'sini yükler.
Bazı CPU exception'larını ölümcül hata olarak raporlar.
PIT timer'ını 100 Hz'e ayarlar.
PIC, LAPIC LINT0 ve IDT üzerinden timer IRQ alır.
Sanal adresi fiziksel adrese çevirebilir.
LAPIC MMIO eşlemesi kurar.
Limine physical memory map'ini yazdırır.
~~~

## Kaynak ağacı

~~~
kernel/src/
├── main.c                 Boot sonrası başlangıç sırası
├── memory.c/.h            Freestanding bellek rutinleri
├── arch/x86_64/           x86-64'e özgü CPU ve interrupt kodu
├── drivers/               COM1, framebuffer ve PIT sürücüleri
└── kernel/                Panic, exception, timer ve font mantığı
~~~

arch/x86_64 içindeki dosyalar başka bir işlemci mimarisinde aynı biçimde
çalışmaz. Örneğin GDT, IDT, PIC ve LAPIC x86 dünyasına aittir. Buna karşılık
kernel/timer.c içindeki ticks sayacı daha taşınabilir bir kernel katmanı olarak
düşünülebilir; onu çağıran interrupt giriş yolu mimariye özgüdür.

## Sorumluluk sınırları

| Dosya veya grup | Sorumluluk |
| --- | --- |
| main.c | Başlatma sırası, Limine response kontrolü, boot logları |
| serial.* | Erken hata ayıklama için COM1 çıktısı |
| gdt.*, idt.*, interrupts.S | CPU'nun segment ve interrupt tabloları |
| pic.*, pit.*, lapic.* | Timer interrupt zincirindeki donanım katmanları |
| paging.* | Mevcut 4 KiB eşlemelerde sanal-fiziksel adres çevirisi |
| memory.* | Derleyicinin isteyebileceği temel C bellek rutinleri |
| panic.*, exception.* | Güvenli devam edilemeyen durumlarda raporlama ve durma |

## Kod ile deney kodu

lapic_debug_pml4_511, lapic_debug_pdpt_510, lapic_debug_pd_0 ve
lapic_debug_pt_0 fonksiyonları, page table yürüyüşünü gözlemlemek için eklenmiş
öğretici debug fonksiyonlarıdır. Kernelin günlük çalışması için genel amaçlı bir
paging arayüzü değillerdir. Buna karşılık paging_translate_4k, LAPIC table
dizilerinin fiziksel adresini bulmak için gerçek başlangıç yolunda kullanılır.

## Kontrol noktası

1. main.c neden bütün donanım sürücülerini doğrudan uygulamaz?
2. Hangi klasör x86-64'e özgü kodu ayırır?
3. Debug fonksiyonları ile kalıcı kernel arayüzü arasındaki fark nedir?
