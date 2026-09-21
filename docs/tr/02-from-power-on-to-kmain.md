# Güç düğmesinden kmain()e

Bu bölüm, KBM çalışmadan önce bilgisayarın hangi katmanlardan geçtiğini anlatır.
Kernel, güç verildiği anda başlayan ilk kod değildir.

## Yeni terimler

- **Firmware:** Anakart flash belleğinde duran ilk açılış yazılımı.
- **BIOS:** Eski PC firmware standardı.
- **UEFI:** Modern firmware standardı.
- **Bootloader:** Kernel'i bulup RAM'e yükleyen ve çalıştırılabilir ortamı
  hazırlayan kısa ömürlü program.
- **Long mode:** x86-64 işlemcinin 64 bit çalışma modu.
- **Limine:** KBM'nin kullandığı bootloader.

## Açılış hikâyesi

~~~
Güç verilir
  -> CPU reset olur
  -> firmware çalışır
  -> firmware bootloader'ı bulur
  -> Limine kernel ELF dosyasını RAM'e yükler
  -> Limine gerekli page table ve boot bilgisini hazırlar
  -> Limine kmain() fonksiyonuna atlar
~~~

CPU, x86 uyumluluğu nedeniyle reset sonrasında eski 16 bit başlangıç ortamında
başlar. BIOS tabanlı boot yolunda Limine bu eski ortamdan gelip kernel için
64 bit long mode ve paging ortamını hazırlar. x86-64 UEFI yolunda UEFI
bootloader'ı zaten 64 bit ortamda çalıştırabilir; yine de Limine kernelin
beklediği bellek düzenini ve response yapılarını kurar.

## BIOS ve UEFI arasındaki fark

Klasik BIOS, boot edilecek disk veya CD'nin ilk boot kodunu çalıştırır. Eski
BIOS programları ekran gibi hizmetleri int 0x10 aracılığıyla firmware'e
sorabilirdi. Bu hizmetler 16 bit real-mode dünyasına aittir.

UEFI ise genellikle GPT diskteki EFI System Partition üzerinde bulunan
BOOTX64.EFI gibi bir EFI executable'ı çalıştırır. UEFI, bellek haritası ve
framebuffer gibi daha modern boot bilgileri sağlayabilir. Secure Boot açıksa
firmware, boot zincirindeki imzaları doğrulayabilir.

KBM ISO'su Limine sayesinde BIOS ve UEFI için boot bileşenleri içerir. Root
Makefile'daki normal make run, özel bir UEFI firmware'i verilmediği için
QEMU'nun varsayılan BIOS yolunu kullanır. make run-uefi ise OVMF tabanlı UEFI
firmware indirilmişse UEFI yolunu kullanır.

## Limine ile KBM arasındaki sözleşme

main.c içindeki .limine_requests bölümü, kernelin Limine'den istediği bilgileri
içerir:

~~~
framebuffer request -> ekrana pixel yazabilmek için framebuffer bilgisi
HHDM request        -> fiziksel RAM'e sabit offset ile erişebilmek için bilgi
memmap request      -> fiziksel adres bölgelerinin envanteri
~~~

used ve section öznitelikleri bu yapıların derleyici tarafından atılmamasını ve
linker'ın doğru bölüme koymasını sağlar. Linker script'teki .limine_requests
bölümü de Limine'in onları bulabilmesini sağlar.

## Kernel neden yüksek adreste başlar?

kernel/linker-scripts/x86_64.lds, kernel bölümlerini
0xffffffff80000000 sanal adresinden başlatır. Bu high-half kernel düzenidir.
Alt sanal adres alanı ileride kullanıcı programları için ayrılabilir; kernel
yüksek alanda sabit bir konumda yaşar. Bu adres fiziksel RAM adresi değildir.
Limine'in kurduğu page table'lar, bu yüksek sanal adresleri kernelin RAM'deki
fiziksel sayfalarına bağlar.

## kmain() başladıktan sonra

Limine kontrolü KBM'ye bırakır. KBM önce seri portu başlatır, Limine response
pointer'larını kontrol eder, sonra kendi GDT ve IDT'sini yükler. Bu noktadan
sonra CPU exception ve interrupt davranışının giderek daha büyük kısmı KBM'nin
kendi kodu tarafından yönetilir.

## Kontrol noktası

1. Firmware ile bootloader arasındaki görev farkı nedir?
2. Long mode neden kernel açısından önemlidir?
3. HHDM ve memory map bilgisi KBM'ye hangi yolla gelir?
4. 0xffffffff80000000 neden fiziksel RAM adresi değildir?
