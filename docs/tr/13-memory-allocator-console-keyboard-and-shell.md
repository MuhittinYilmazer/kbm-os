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

`kmalloc(size)` isteği 16 byte'a yuvarlar. Mevcut 4 KiB heap frame'inde yer
yoksa PMM'den yeni frame alır ve physical address + HHDM offset ile C pointer'ı
oluşturur. Bu bir bump allocator'dır: `kfree` yoktur ve 4 KiB'tan büyük tek
istek panic verir. Bu, v0.1'in bilinçli sınırıdır.

`heap_get_used_bytes` ve `heap_get_frame_count`, boot testleriyle `MEM`
komutunun heap'i gözlemlemesini sağlar.

## Doğrulama

`main.c` boot sırasında şunları test eder:

- PMM allocation/free sonrası free-frame sayısı geri döner.
- Free edilmiş frame tekrar bulunabilir.
- Heap ilk frame dolunca PMM'den ikinci frame alır.
- `kmalloc(13)` 16-byte hizalı pointer verir ve kullanım sayacını 16 artırır.

~~~text
KBM_PMM_OWNER_TEST_OK
KBM_PMM_FREE_REUSE_OK: ...
KBM_HEAP_SECOND_FRAME_TEST_OK: ...
KBM_HEAP_ACCOUNTING_TEST_OK
~~~

## Console, klavye ve shell

İlgili dosyalar: `drivers/framebuffer.*`, `drivers/console.*`,
`kernel/font.*`, `drivers/keyboard.*`, `kernel/shell.c`.

Console, Limine framebuffer'ına 8×8 bitmap glyph çizer; satır taşınca sarar;
ekran dolunca scroll etmek yerine temizler. Font büyük ASCII, rakamlar,
noktalama ve gerekli Türkçe karakter alt kümesini içerir. UTF-8 desteği sadece
KBM'nin kullandığı ASCII ve iki-byte Türkçe diziler içindir.

PS/2 tuş basımı IRQ1 üretir. PIC remap sonrası IDT vector 33'e gelir.
`isr_keyboard` C ABI stack hizalamasını kurar. `keyboard_irq`, port `0x60`dan
Set 1 scan code okur, küçük Türkçe-Q eşlemesiyle shell'e karakter verir ve EOI
gönderir. v0.1 yalnızca Shift durumunu izler.

Shell en fazla 63 codepoint tutar. Komutlar: `HELP`, `CLEAR`, `MEM`, `TICKS`,
`ECHO metin` ve `ZEYNEP` easter egg'idir. Sayılar şimdilik hexadecimaldir.

## Kontrol noktası

1. Primary ve ownership bitmap hangi farklı soruyu cevaplar?
2. Heap neden physical frame adresine HHDM offset ekler?
3. `kmalloc(13)` neden 16 byte tüketir?
4. Tuş basımı IDT'den shell'e hangi zincirle ulaşır?
