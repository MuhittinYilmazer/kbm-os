# Fiziksel bellek, sanal bellek ve paging

Bir kernel pointer kullandığında CPU çoğu zaman bu değeri doğrudan RAM adresi
olarak kullanmaz. Pointer bir **sanal adres**tir. CPU page table'lara bakarak
onu fiziksel RAM adresine veya bir cihaz adresine çevirir.

## Yeni terimler

- **Fiziksel adres:** RAM, firmware veya cihazlar için CPU'nun fiziksel adres
  alanındaki adres.
- **Sanal adres:** Kernelin veya ileride bir process'in kullandığı adres.
- **Physical frame:** Fiziksel RAM'deki 4 KiB blok.
- **Virtual page:** Sanal adres alanındaki 4 KiB blok.
- **Page table:** Sanal page'i fiziksel frame'e bağlayan tablo.
- **CR3:** Aktif PML4 table'ının fiziksel başlangıç adresini tutan control
  register.
- **HHDM:** Higher-Half Direct Map; fiziksel RAM'e sabit offset ekleyerek
  erişme düzeni.
- **TLB:** CPU'nun yakın zamanda kullandığı adres çevirilerini tuttuğu hızlı
  önbellek.

## Fiziksel adres alanı yalnızca RAM değildir

CPU'nun fiziksel adres alanında RAM yanında firmware, ACPI tabloları, PCI
cihazları, framebuffer ve LAPIC gibi MMIO cihazları da bulunabilir. Bu nedenle
rastgele fiziksel adrese yazmak güvenli değildir.

Limine memory map, fiziksel alanın büyük aralıklarla envanterini verir. Örneğin
USABLE türü normal RAM'i, FRAMEBUFFER türü görüntü belleğini, RESERVED türü ise
dokunulmaması gereken alanları anlatır. Bir memory-map entry milyonlarca 4 KiB
frame'i kapsayan büyük bir aralık olabilir; her frame için ayrı satır değildir.

## Page ve frame

KBM'nin normal paging yolu 4 KiB birim kullanır:

~~~
virtual page  -> physical frame
4 KiB          -> 4 KiB
~~~

Örnek olarak kernelin sanal adresi 0xffffffff80000020 olabilir. Page table bu
adresin içinde bulunduğu virtual page'i fiziksel 0x7ff41000 frame'ine
bağlarsa, son 12 bit olan 0x20 page içi offset korunur:

~~~
0x7ff41000 + 0x20 = 0x7ff41020
~~~

Bu nedenle son 12 bit offset'tir: 2^12 = 4096 byte.

## Neden dört seviye table var?

64 bit sanal adres alanı çok büyüktür. Her olası 4 KiB page için tek dev bir
düz tablo ayırmak yüzlerce GiB bellek isteyebilirdi. x86-64, adresi parçalara
böler ve yalnızca kullanılan bölgeler için alt table'lar oluşturur:

~~~
| PML4 index | PDPT index | PD index | PT index | offset |
|    9 bit   |    9 bit   |   9 bit  |   9 bit  | 12 bit |
~~~

Her table 512 giriş içerir. Her giriş 8 byte olduğundan bir table tam 4096
byte, yani bir physical frame büyüklüğündedir.

~~~
CR3
 -> PML4
    -> PDPT
       -> PD
          -> PT
             -> physical frame
~~~

## CR3 ve HHDM

CR3, PML4'ün fiziksel adresini tutar. CPU çeviriye sanal adres kullanmadan
başlayabilmek için fiziksel kök adresine ihtiyaç duyar; aksi halde çeviriyi
bulmak için yine çeviri gerekirdi.

C kodu fiziksel adresi normal pointer gibi kullanamaz. main.c'de Limine'den
istenen HHDM response, fiziksel **RAM** için bir offset verir:

~~~
RAM virtual address = HHDM offset + RAM physical address
~~~

Bu formül page table'ların kendisine erişmek için kullanışlıdır; table'lar
normal RAM'de yaşar. LAPIC gibi cihazlar RAM değildir, bu nedenle HHDM tek
başına LAPIC erişimi sağlamaz.

## paging_translate_4k

paging.c içindeki paging_translate_4k, CPU'nun basitleştirilmiş table
yürüyüşünü C ile tekrarlar.

1. Sanal adresten dört adet 9 bit index ve 12 bit offset çıkarır.
2. CR3'ü okur ve HHDM ile PML4 pointer'ına ulaşır.
3. PML4, PDPT, PD ve PT girişlerini sırayla okur.
4. Her girişte PRESENT bitini kontrol eder.
5. PT girişinin frame adresiyle orijinal offset'i birleştirir.

Girişlerde adres ve bayraklar aynı 64 bit değerde bulunur. ADDRESS_MASK,
adres alanını alıp düşük bayrak bitlerini temizler. PAGE_PRESENT bit 0'dır.

Bu fonksiyon PDPT veya PD girişinde PAGE_HUGE bitini görürse false döner.
Bu bir hata değil, açık bir kapsam sınırıdır: fonksiyon yalnızca normal 4 KiB
page zincirini çözmeyi öğretmek ve kullanmak için yazılmıştır; 1 GiB veya 2 MiB
huge page çevirisini henüz desteklemez.

## Doğrulama

main.c, hhdm_request değişkeninin sanal adresini paging_translate_4k ile
fiziksel adrese çevirir ve KBM_HHDM_REQUEST_PHYS marker'ını yazar. Debug
fonksiyonları ayrıca kernelin high-half başlangıç adresi için PML4[511],
PDPT[510], PD[0] ve PT[0] girişlerini yazdırır.

Bu çıktıların sayısal adresleri QEMU veya kernel düzeni değiştiğinde farklı
olabilir. Önemli olan zincirin present girişlerden geçmesi ve sonuçta fiziksel
frame ile offset'in doğru birleşmesidir.

## Kontrol noktası

1. Physical frame ile virtual page arasındaki fark nedir?
2. CR3 neden fiziksel adres tutar?
3. HHDM hangi tür fiziksel alan için kullanılabilir?
4. Dört seviyeli yapı neden tek düz tablodan daha verimlidir?
5. paging_translate_4k neden huge page görünce false döner?
