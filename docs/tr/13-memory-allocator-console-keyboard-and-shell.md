# 13 — PMM, heap, console, klavye ve shell

KBM artık yalnızca seri log yazan kernel değildir. Limine memory map'ten
fiziksel frame ayırır, küçük heap allocation'ları yapar, framebuffer'a metin
çizer ve PS/2 klavyeden gelen karakterlerle shell çalıştırır.

## Çalışma zinciri

~~~text
Limine memory map → PMM → kmalloc heap → framebuffer console
                                      ↑             ↓
                             PS/2 IRQ1 keyboard → shell
~~~

PMM fiziksel RAM'in sahipliğini yönetir. Heap PMM'den frame ister. Console
piksel belleğine yazar. Klavye interrupt üretir. Shell karakterleri komuta
dönüştürür.

## PMM ve iki bitmap

İlgili dosyalar: `kernel/src/kernel/pmm.c`, `pmm.h`, `main.c`.

KBM'nin birimi 4 KiB fiziksel frame'dir. Her frame için iki bit tutulur:

~~~text
primary bitmap:   0 = boş / PMM verebilir, 1 = kullanılmış veya reserved
ownership bitmap: 0 = PMM çağırana vermedi, 1 = PMM çağırana verdi
~~~

İkinci bitmap yanlış `free` çağrılarını yakalar. Reserved bir frame veya daha
önce free edilmiş frame için ownership biti sıfırdır. `pmm_free_frame` bu
durumda `KBM_FRAME_NOT_ALLOCATED` panic'i verir.

Başlangıçta PMM, `LIMINE_MEMMAP_USABLE` aralıklarını bulur; en yüksek usable
adrese göre iki bitmapin boyutunu hesaplar; bitmaplerin frame'lerini geçici
bump allocator ile ayırır; tam 4 KiB usable frame'leri boş işaretler; metadata
frame'lerini tekrar reserved yapar.

## Heap

İlgili dosyalar: `kernel/src/kernel/heap.c`, `heap.h`.

`kmalloc(size)` isteği 16 byte'a yuvarlar, sonra adres sıralı free-list içinde
uygun blok arar. Her boş blokta payload boyutu, sonraki blok pointer'ı, boşluk
durumu ve magic değeri bulunan küçük bir başlık vardır. Uygun blok yoksa heap
PMM'den 4 KiB frame ister ve physical address + HHDM offset ile C pointer'ı
oluşturur. Başlık çıkarıldıktan sonra tek frame'e sığmayan istek panic verir.

`kfree(pointer)`, çağırana verilmiş payload pointer'ından bir başlık geri gider;
magic değerini ve double-free durumunu kontrol eder, sonra bloğu free-list'e
geri koyar. Liste adres sıralı tutulduğu için fiziksel olarak bitişik boş
bloklar birleştirilebilir. Bu fragmentation'ı azaltır. Tamamen boş kalan heap
frame'i henüz PMM'ye geri verilmez.

`heap_get_used_bytes` ve `heap_get_frame_count`, boot testleriyle `MEM`
komutunun heap'i gözlemlemesini sağlar.

## Doğrulama

`main.c` boot sırasında şunları test eder:

- PMM allocation/free sonrası free-frame sayısı geri döner.
- Free edilmiş frame tekrar bulunabilir.
- Heap ilk frame dolunca PMM'den ikinci frame alır.
- `kmalloc(13)` 16-byte hizalı pointer verir ve kullanım sayacını 16 artırır.
- `kfree` kullanılan byte sayısını düşürür; daha sonraki büyük istek, birleşmiş boş alanı yeni PMM frame'i almadan kullanır.

~~~text
KBM_PMM_OWNER_TEST_OK
KBM_PMM_FREE_REUSE_OK: ...
KBM_HEAP_SECOND_FRAME_TEST_OK: ...
KBM_HEAP_ACCOUNTING_TEST_OK
KBM_HEAP_FREE_MERGE_OK
~~~

## Console, klavye ve shell

İlgili dosyalar: `drivers/framebuffer.*`, `drivers/console.*`,
`kernel/font.*`, `drivers/keyboard.*`, `kernel/shell.c`.

Console, Limine framebuffer'ına 8×8 bitmap glyph çizer; satır taşınca sarar ve
cursor alta ulaşınca bir glyph satırı yukarı scroll eder. `screen_scroll_up`,
framebuffer satırlarını yukarı kopyalar ve altta açılan satırı temizler. Font
büyük ASCII, rakamlar, noktalama ve gerekli Türkçe karakter alt kümesini içerir.
UTF-8 desteği sadece KBM'nin kullandığı ASCII ve iki-byte Türkçe diziler içindir.

PS/2 tuş basımı IRQ1 üretir. PIC remap sonrası IDT vector 33'e gelir.
`isr_keyboard` C ABI stack hizalamasını kurar. `keyboard_irq`, port `0x60`dan
Set 1 scan code okur, küçük Türkçe-Q eşlemesiyle shell'e karakter verir ve EOI
gönderir. v0.1 yalnızca Shift durumunu izler.

Shell en fazla 63 codepoint tutar. Komutlar: `HELP`, `CLEAR`, `MEM`, `TICKS`,
`UPTIME`, `ECHO metin`, `REBOOT` ve `KOCAELI`dir. Sayı ve uptime çıktısında
decimal kullanılır; hexadecimal ise bring-up aşamasında adresler ve bit alanları
için hâlâ yararlıdır.

`UPTIME`, PIT'in saniyede yaklaşık 100 tick üretmesini tam saniyeye çevirir.
`REBOOT`, `arch/x86_64/reboot.c` içinden legacy keyboard controller'a `0x64`
I/O portu üzerinden CPU reset isteği yollar. Bu komutu `make run` ile dene;
serial-debug hedefi bilerek QEMU'nun `-no-reboot` seçeneğini kullanır ve guest
reboot isteğinden sonra kapanır.

## Kontrol noktası

1. Primary ve ownership bitmap hangi farklı soruyu cevaplar?
2. Heap neden physical frame adresine HHDM offset ekler?
3. `kmalloc(13)` neden 16 byte tüketir?
4. Tuş basımı IDT'den shell'e hangi zincirle ulaşır?
