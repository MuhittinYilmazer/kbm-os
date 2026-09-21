# LAPIC, LINT0 ve MMIO

KBM timer kodu ilk başta PIT'i ayarlayıp PIC'te IRQ0 maskesini kaldırmasına
rağmen tick almıyordu. Sorun PIT veya IDT değildi: eski PIC sinyali LAPIC'e
kadar geliyordu, fakat LAPIC'in LINT0 girişi maskeliydi.

Bu bölüm, sorunu ve çözümü bütün yoluyla anlatır.

## Yeni terimler

- **APIC:** Advanced Programmable Interrupt Controller ailesi.
- **LAPIC:** Local APIC; CPU çekirdeğine yakın interrupt denetleyicisi.
- **LINT0:** Local Interrupt input 0; legacy PIC çıkışının LAPIC'e girdiği hat.
- **ExtINT:** LINT0 için eski PIC interrupt'ını kabul eden delivery mode.
- **MMIO:** Memory-Mapped I/O; cihaz register'larına bellek adresi gibi erişim.
- **PCD/PWT:** Page table cache davranışı bitleri.
- **invlpg:** Bir sanal adresin TLB çevirisini geçersiz kılan x86 talimatı.

## APIC katmanı nerede durur?

Bugünkü legacy timer yolu şöyledir:

~~~
PIT
 -> IRQ0
master PIC
 -> legacy interrupt output
LAPIC LINT0
 -> CPU
IDT[32]
 -> isr_timer
~~~

PIC eski bir interrupt denetleyicisidir. LAPIC ise modern x86 CPU'larda bulunan
yerel interrupt denetleyicisidir. Çok çekirdekli sistemde her çekirdeğin kendi
LAPIC'i vardır. I/O APIC, dış cihaz interrupt'ları için daha modern başka bir
bileşendir; KBM şu anda I/O APIC kullanmaz.

QEMU incelemesinde PIC'in IRQ0'ı pending tuttuğu, fakat LAPIC LINT0'ın masked
ExtINT olduğu görülmüştü. Bu nedenle isr_timer'a hiç girilmiyordu.

## LAPIC neden özel eşleme ister?

LAPIC'in fiziksel tabanı:

~~~
0xFEE00000
~~~

Bu normal RAM değildir; cihaz register'larının fiziksel MMIO alanıdır. HHDM
yalnızca fiziksel RAM'i sanal adrese taşır. Bu nedenle HHDM offset + LAPIC
physical base işlemi güvenilir bir LAPIC pointer'ı değildir.

KBM, LAPIC için kendi sanal adresini seçer:

~~~
LAPIC virtual base:  0xFFFFFFFFC0000000
LAPIC physical base: 0x00000000FEE00000
~~~

Amaç şudur:

~~~
Kernel 0xFFFFFFFFC0000000 adresine erişirse
  -> CPU page table'ları yürüsün
  -> sonuç 0xFEE00000 olsun
  -> işlem LAPIC cihazına gitsin
~~~

## Yeni page table'lar

lapic.c içindeki iki static, 4096-byte-aligned dizi başlangıçta normal RAM
verisidir:

~~~
lapic_page_directory[512]
lapic_page_table[512]
~~~

Her biri 512 adet 8 byte giriş içerir; böylece tam bir 4 KiB page table
büyüklüğündedir. lapic_enable_legacy_pic önce paging_translate_4k ile bu
dizilerin fiziksel adresini bulur. Page table girişlerinin sonraki table için
fiziksel adres istemesi nedeniyle bu adım zorunludur.

CR3 ve HHDM üzerinden mevcut PML4 bulunur. Kernelin high-half alanı zaten
PML4[511] altında bulunduğundan bu giriş mevcut PDPT'ye gider. KBM, seçilen
LAPIC sanal adresi için boş olması gereken PDPT[511] girişini kullanır:

~~~
PML4[511]                  -> Limine'in mevcut PDPT'si
PDPT[511]                  -> lapic_page_directory physical address
lapic_page_directory[0]    -> lapic_page_table physical address
lapic_page_table[0]        -> 0xFEE00000 + page flags
~~~

Fonksiyon, PDPT[511] zaten present ise false döner. Bu, seçilmiş sanal alanın
başka bir eşlemeyi ezmesini engelleyen basit bir güvenlik kontrolüdür.

## MMIO page flags ve TLB

Ara table girişleri PRESENT | WRITABLE ile kurulur. Son LAPIC PTE'si bunlara ek
olarak PAGE_WRITE_THROUGH ve PAGE_CACHE_DISABLE kullanır. Amaç cihaz
register'larının normal cache'lenebilir RAM gibi ele alınmamasıdır.

Yeni çeviri yazıldıktan sonra:

~~~
invlpg(LAPIC_VIRTUAL_BASE)
~~~

çağrılır. Bu, CPU'ya bu sanal adres için eski TLB bilgisini unutmasını söyler.
CPU LAPIC adresine ilk kez erişecek olsa bile, mapping değişikliğinden sonra
invlpg çağırmak güvenli ve öğretici bir alışkanlıktır.

lapic pointer'ı volatile uint32_t türündedir. Cihaz register'ı okuması veya
yazması gözlemlenebilir donanım işlemi olduğundan derleyici bu erişimleri RAM
okuması gibi optimize edip silemez.

## LINT0 ayarı

LAPIC LINT0 register offset'i 0x350'dir. Pointer uint32_t olduğundan kod
lapic[0x350 / 4] ile bu 32 bit register'a ulaşır.

Kod mevcut değeri okuyup yalnızca gerekli alanları değiştirir:

~~~
bit 8-10  -> delivery mode, 111 yapılır: ExtINT
bit 16    -> mask, 0 yapılır: unmasked
~~~

Bu ayar LAPIC'e şunu söyler:

~~~
Legacy PIC'ten LINT0 üzerinden gelen interrupt'ları kabul et.
~~~

PIT veya PIC'i açan kod bu değildir. PIT pit_init ile programlanır; PIC IRQ0
maskesi pic_enable_irq(0) ile açılır. LINT0, bu iki eski cihaz ile CPU arasında
eksik kalan köprüyü açar.

## Kanıt ve sınırlar

KBM_PIT_TICKS: 0x64 çıktısı 100 timer interrupt işlendiğini gösterir. Bu çıktı
şu bileşenlerin birlikte çalıştığını kanıtlar:

~~~
PIT -> PIC -> LINT0 -> LAPIC -> CPU -> IDT -> ISR -> timer_irq -> EOI -> iretq
~~~

Bu LAPIC kodu bir bootstrap çözümüdür. Sabit fiziksel LAPIC tabanı ve sabit
seçilmiş sanal adres kullanır; I/O APIC, x2APIC, LAPIC timer, SMP ve genel amaçlı
MMIO mapper henüz yoktur.

## Kontrol noktası

1. HHDM neden LAPIC'e erişmek için tek başına yeterli değildir?
2. lapic_page_directory neden normal bir C dizisi olarak başlar?
3. Page table girişleri neden virtual değil physical address saklar?
4. LINT0'daki ExtINT ve unmasked ayarları ne anlama gelir?
5. KBM_PIT_TICKS marker'ı hangi zinciri kanıtlar?
