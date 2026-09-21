# KBM'yi derlemek ve QEMU'da çalıştırmak

KBM bir freestanding ELF kernel olarak derlenir, Limine dosyalarıyla bootable
ISO içine yerleştirilir ve QEMU'da çalıştırılır.

## Yeni terimler

- **Toolchain:** Derleyici, assembler ve linker gibi derleme araçları.
- **ELF:** x86-64 Unix benzeri sistemlerde yaygın executable biçimi.
- **Linker:** Object dosyalarını birleştirip adres düzenini kuran araç.
- **ISO:** CD-ROM benzeri boot image.
- **QEMU:** Donanımı taklit eden emulator veya sanallaştırıcı.

## Normal build

Repo kökünde:

~~~
make all
~~~

Bu komut önce kernel içindeki Makefile ile kernel ELF dosyasını üretir.
Ardından root Makefile şunları yapar:

~~~
kernel/bin/kernel dosyasını ISO ağacına kopyalar
Limine BIOS ve UEFI dosyalarını ekler
xorriso ile kbm.iso oluşturur
limine bios-install ile BIOS boot bilgisini yerleştirir
~~~

Son ürün repo kökündeki kbm.iso dosyasıdır.

## QEMU ile çalıştırmak

Grafik pencere için:

~~~
make run
~~~

Erken kernel loglarını terminalde görmek için seri portu standart girdiye ve
çıktıya bağlayan komut daha kullanışlıdır:

~~~
qemu-system-x86_64 -M q35 -cdrom kbm.iso -boot d -m 2G \
  -display none -serial stdio -monitor none -no-reboot
~~~

Buradaki -m 2G, QEMU içindeki sanal makineye 2 GiB RAM verir. Bu, host
bilgisayarın fiziksel RAM miktarı değildir.

## Derleme katmanları

kernel/GNUmakefile bütün src ağacındaki .c ve .S dosyalarını bulur. C dosyaları
freestanding bayraklarla derlenir:

~~~
-ffreestanding  -> hosted C ortamı varsayma
-nostdinc       -> host standart include dizinlerini kullanma
-m64            -> x86-64 kod üret
-mno-red-zone   -> interrupt kullanan kernel için güvenli ABI seçimi
~~~

Linker script, kmain'i entry point yapar ve kernel bölümlerinin high-half sanal
adreslerini belirler.

## Başarılı boot işaretleri

Mevcut kernelin seri portta vermesi beklenen önemli marker'lar:

~~~
KBM_BOOT_OK
KBM_IDT_LOADED
KBM_LAPIC_MMIO_MAPPED
KBM_PIC_REMAP_OK
KBM_PIT_TICKS: 0x0000000000000064
KBM_MEMMAP_ENTRIES: ...
~~~

KBM_PIT_TICKS satırı, timer interrupt zincirinin en az 100 kez çalıştığını
gösterir. Memory-map satırları bu aşamadan sonra yazdırılır.

## Kontrol noktası

1. make all sonucunda hangi boot image dosyası oluşur?
2. -m 2G hangi belleği belirler?
3. Neden serial output kernel geliştirmede özellikle değerlidir?
