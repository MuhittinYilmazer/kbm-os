# CPU durumu, GDT ve stack

KBM'nin C kodu çalışırken CPU yalnızca talimat yürütmez; register, stack,
segment seçicileri, privilege kuralları ve page table kökü gibi bir çalışma
durumu taşır. GDT bu durumun tarihsel ama hâlâ gerekli parçalarından biridir.

## Yeni terimler

- **Register:** CPU'nun içindeki çok küçük ve hızlı değer saklama alanı.
- **RIP:** Çalıştırılacak sonraki talimatın sanal adresi.
- **RSP:** Stack'in tepesini gösteren stack pointer.
- **Segment selector:** GDT içindeki bir girişi seçen küçük sayı.
- **GDT:** Global Descriptor Table; segment kurallarını tanımlayan CPU tablosu.
- **Ring 0:** Kernel privilege seviyesi.
- **Long mode:** x86-64'ün 64 bit çalışma modu.

## Register ve stack

C fonksiyon çağrıları görünürde basittir, ancak CPU register ve stack kullanır.
x86-64 System V ABI'de ilk fonksiyon argümanı RDI register'ı ile verilir.
Bu nedenle gdt_load fonksiyonuna verilen GDT descriptor pointer'ı assembly
tarafında RDI içindedir.

Stack, geçici veriler ve dönüş adresleri için LIFO yapısıdır:

~~~
push value  -> RSP 8 byte azalır, değer stack'e yazılır
pop value   -> değer stack'ten okunur, RSP 8 byte artar
call target -> dönüş adresini stack'e koyar, target'a gider
ret         -> dönüş adresini stack'ten alır
~~~

64 bit push ve pop işlemleri 8 byte hareket eder. Interrupt geldiğinde CPU da
dönüş için gerekli bazı bilgileri stack'e koyar; bunun ayrıntısı sonraki
bölümdedir.

## Segment nedir?

Eski x86 işlemcilerde segmentler adres alanını bölmek için kullanılıyordu.
64 bit long mode'da normal kod ve veri erişimlerinde segment taban adresleri
büyük ölçüde kullanılmaz; paging asıl bellek koruma ve adres çevirme işini
yapar. Buna rağmen CPU, CS, DS, ES ve SS gibi segment register'larında geçerli
selector'lar bekler. Ayrıca code segment'in long-mode ve privilege nitelikleri
önemini korur.

KBM'nin minimal GDT'si üç girişten oluşur:

~~~
GDT[0] -> null entry, zorunlu olarak sıfır
GDT[1] -> Ring 0 64 bit code segment, selector 0x08
GDT[2] -> Ring 0 data/stack segment, selector 0x10
~~~

Selector değeri bir byte adresi değildir. GDT içindeki giriş numarasıyla
ilişkilidir. Girişler 8 byte olduğundan birinci kullanılabilir giriş 0x08,
ikinci giriş 0x10 seçicisini kullanır.

## gdt.c ne yapar?

gdt.c, CPU'nun beklediği 8 byte'lık gdt_entry biçimini ve 10 byte'lık
gdt_descriptor biçimini tanımlar. gdt_init içinde static dizi kullanılması
önemlidir: fonksiyon bittikten sonra CPU GDT'yi kullanmaya devam eder, bu
nedenle tablo stack üzerinde geçici olamaz.

Code entry için access değeri 0x9A, data entry için 0x92 atanır. Bu değerler
present, Ring 0, code/data ve writable gibi alanları bitler halinde taşır.
Code entry'deki flags_limit değeri 0x20, bunun 64 bit code segment olduğunu
belirtir.

## gdt_load.S ne yapar?

C dili CS register'ını doğrudan yeniden yükleyemez. Bu nedenle assembly gerekir:

~~~
cli
lgdt [rdi]
push 0x08
push gdt_loaded adresi
lretq
gdt_loaded:
  DS, ES ve SS'yi 0x10 ile yükle
  ret
~~~

lgdt, GDT descriptor'ını CPU'ya yükler; ancak mevcut CS eski selector olarak
kalır. lretq bir far return'dür: stack'teki yeni instruction address ve code
selector'ı kullanarak CS'yi de değiştirir. Ardından data ve stack selector'ları
yenilenir.

GDT yüklenirken cli ile maskelenebilir interrupt'lar kapalı tutulur. Aksi halde
CPU tabloları yarı güncellenmişken bir interrupt gelmesi istenmez.

## Sınırlar

KBM yalnızca Ring 0 kernel segmentleri kurar. Henüz Ring 3 kullanıcı modu,
TSS, IST stack'leri veya kullanıcı/kernel geçişi yoktur. Long mode'da segment
tabanları yerine paging kullanıldığı için bu minimal GDT başlangıç için yeterli
olabilir, fakat tam bir kullanıcı modu sistemi için yeterli değildir.

## Kontrol noktası

1. RSP neden stack pointer olarak önemlidir?
2. 0x08 ve 0x10 neden GDT selector'larıdır?
3. lgdt neden CS'yi tek başına güncellemez?
4. Long mode'da paging varken GDT tamamen neden ortadan kalkmaz?
