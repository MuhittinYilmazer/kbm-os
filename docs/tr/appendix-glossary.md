# Ek — KBM kavram sözlüğü

Bu sözlük bölümlerin yerine geçmez; okurken karşılaştığın sözcüğü hızlıca yerine koymak içindir. Bir terim ilk kez geçtiği bölümde açıklanır, burada kısa başvuru karşılığını bulursun.

| Terim | Kısa anlamı |
| --- | --- |
| **ABI** | İkili kodların birlikte çalışma sözleşmesi: register kullanımı, stack hizası, parametre ve dönüş değerleri gibi kurallar. |
| **ACPI** | Firmware'in işletim sistemine güç yönetimi ve donanım yapılandırması hakkında verdiği standart tablolar. |
| **APIC** | Advanced Programmable Interrupt Controller. Modern interrupt yönlendirme sisteminin genel adı. |
| **BIOS** | Geleneksel PC firmware'i. Eski boot ortamıdır; 16-bit servis interrupt'larıyla ilişkilidir. |
| **bootloader** | Firmware'den sonra çalışan, kernel imajını belleğe yükleyip kontrolü kernela veren program. KBM'de Limine. |
| **CPL / ring** | CPU privilege seviyesi. Ring 0 kernel, ring 3 kullanıcı programları için tipik seviyedir. |
| **COM1 / UART** | PC'deki ilk klasik seri port ve onu sağlayan iletişim donanımı. KBM logları buradan çıkar. |
| **CR3** | x86-64 control register 3. Aktif üst seviye page table'ın (PML4) fiziksel adresini taşır. |
| **CS, DS, SS** | Code, data ve stack segment selector register'ları. Long mode'da taban/limit rolü büyük ölçüde kaybolsa da selector ve privilege kuralları sürer. |
| **ELF** | Kernel gibi programların kod, veri ve yükleme bilgilerini taşıyan yürütülebilir dosya biçimi. |
| **EOI** | End Of Interrupt. PIC/APIC'e “bu interrupt işlendi” bildirimi. |
| **exception** | CPU'nun mevcut talimatın sonucu olarak ürettiği olay: örneğin page fault veya invalid opcode. |
| **frame** | Fiziksel bellekteki sabit boyutlu blok; burada genellikle 4 KiB. Sanal sayfanın fiziksel karşılığıdır. |
| **framebuffer** | Ekranın piksel belleği. Pixel değerini yazmak görüntüyü değiştirir. |
| **GDT** | Global Descriptor Table. Segment selector'larının hangi code/data descriptor'a karşılık geldiğini tanımlar. |
| **HHDM** | Higher-Half Direct Map. Fiziksel RAM'i sabit bir sanal offset ile erişilebilir yapan eşleme. |
| **IDT** | Interrupt Descriptor Table. Vector numarasını handler adresi ve gate kurallarıyla eşler. |
| **IF** | RFLAGS içindeki Interrupt Flag. 1 ise maskelenebilir interrupt'lar CPU tarafından kabul edilir; `sti` açar, `cli` kapatır. |
| **invlpg** | Belirli bir sanal adresin TLB girişini geçersiz kılan x86 talimatı. Page table değişiminden sonra gerekir. |
| **interrupt** | Donanım ya da yazılım kaynaklı asenkron/senkron kontrol aktarımı. CPU IDT'de bir vector handler'ına gider. |
| **I/O port** | `in`/`out` talimatlarıyla erişilen x86 aygıt register adres alanı. PIC, PIT ve COM1 bunu kullanır. |
| **IRQ** | Interrupt Request. Bir donanım aygıtının interrupt isteği; eski PIC dünyasında IRQ0 PIT timerdır. |
| **ISR** | Interrupt Service Routine. Bir interrupt/exception geldiğinde çalışan handler kodu. |
| **iretq** | 64-bit interrupt return talimatı. CPU'nun interrupt frame'inden RIP/CS/RFLAGS ve gerekiyorsa RSP/SS'yi geri yükler. |
| **kernel** | Makinenin en ayrıcalıklı yazılım katmanı; CPU, bellek ve aygıtlar üzerinde doğrudan kuralları uygular. |
| **LAPIC** | Local APIC. Her CPU çekirdeğine bağlı yerel interrupt denetleyicisi. |
| **LINT0** | LAPIC'in Local Interrupt 0 girişidir. Legacy PIC interrupt yolunda ExtINT için kullanılabilir. |
| **Limine** | KBM'nin kullandığı boot protokolü ve bootloader. Kernelin istediği bilgileri request/response yapılarıyla sağlar. |
| **long mode** | x86-64'ün 64-bit çalışma modu. 64-bit register'lar, 4 seviyeli paging ve farklı segment davranışı getirir. |
| **MMIO** | Memory-Mapped I/O. Aygıt register'larının bellek adresi gibi load/store ile erişilmesi. LAPIC böyledir. |
| **page** | Sanal adres alanındaki sabit boyutlu blok; burada 4 KiB. |
| **page table** | Sanal sayfaları fiziksel framelere çeviren tablolardan biri. x86-64'te PML4→PDPT→PD→PT katmanları vardır. |
| **PML4 / PDPT / PD / PT** | 4 seviyeli x86-64 page-table ağacının sırayla üstten alta katmanları. Her giriş sonraki tabloyu veya son seviyede frame'i gösterir. |
| **PIC** | Programmable Interrupt Controller. KBM'nin kullandığı klasik 8259 uyumlu interrupt denetleyicisi. |
| **PIT** | Programmable Interval Timer. Periyodik timer IRQ0 üreten klasik PC aygıtı. |
| **PTE** | Page Table Entry. Son seviye page-table girişi; bir sanal 4 KiB sayfayı fiziksel frame'e bağlar. |
| **RFLAGS** | CPU durum bayraklarını taşıyan register. IF bu register içindedir. |
| **RIP / RSP** | Sırasıyla yürütülecek talimat adresi ve stack pointer. |
| **selector** | CS/DS/SS gibi segment register'ında bulunan, GDT/LDT içindeki descriptor'ı seçen 16-bit değer. |
| **serial log** | Kernelin COM1 üzerinden terminale yazdığı hata ayıklama çıktısı. |
| **stack** | Fonksiyon çağrıları, dönüş adresleri, yerel veriler ve interrupt frame'leri için kullanılan LIFO bellek alanı. x86-64'te RSP ile izlenir. |
| **sti / cli / hlt** | Sırasıyla maskelenebilir interrupt'ları açar, kapatır ve CPU'yu bir interrupt'a kadar bekletir. |
| **TLB** | Translation Lookaside Buffer. CPU'nun yakın zamanlı virtual→physical çevirilerini önbelleğe aldığı yapı. |
| **UEFI** | BIOS'un modern firmware arayüzü. Disk, boot manager ve güvenlik tarafında farklı bir başlangıç ortamı sağlar. |
| **vector** | IDT içinde handler seçmek için kullanılan 0–255 numarası. Örn. #UD 6, PIC IRQ0 remap sonrası 32. |
| **virtual address** | Kodun kullandığı adres. CPU page table yardımıyla fiziksel adrese çevirir. |
| **physical address** | RAM, MMIO ya da başka donanımın gördüğü makine adresi. |

## Sık karışan çiftler

| Karışan kavramlar | Ayıran cümle |
| --- | --- |
| physical memory map / page table | İlki hangi fiziksel alanın ne olduğunu, ikincisi hangi sanal adresin nereye gittiğini söyler. |
| page / frame | Page sanal kutu, frame fiziksel kutudur. |
| exception / IRQ | Exception mevcut talimattan doğar; IRQ dış donanım isteğidir. |
| interrupt / ISR | Interrupt olaydır; ISR onu işleyen koddur. |
| HHDM / allocator | HHDM erişim sağlar; allocator sahiplik ve dağıtım sağlar. |
| PIC / LAPIC | PIC legacy aygıt interrupt'larını toplar; LAPIC bunları CPU çekirdeğine teslim eden modern yerel taraftır. |
| BIOS / bootloader | BIOS firmware'dir; bootloader firmware'den sonra kernel yükleyen yazılımdır. |
| GDT / IDT | GDT segment descriptor'larını, IDT interrupt girişlerini tanımlar. |
