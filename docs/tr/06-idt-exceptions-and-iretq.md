# IDT, exception ve iretq

GDT CPU'nun segment kurallarını tanımlar. IDT ise CPU'ya bir exception veya
interrupt geldiğinde hangi kodun çalışacağını söyler.

## Yeni terimler

- **IDT:** Interrupt Descriptor Table; 256 olası vector için handler tablosu.
- **Vector:** CPU'nun handler seçmek için kullandığı 0-255 numarası.
- **Exception:** CPU'nun kendi yürütmesi sırasında ürettiği olay.
- **IRQ:** Dış donanımın ürettiği interrupt isteği.
- **ISR:** Interrupt Service Routine; CPU'nun girdiği handler.
- **iretq:** 64 bit interrupt dönüş komutu.

## Exception ve IRQ farkı

Bir exception CPU'nun kendi komut yürütmesinden doğar:

~~~
0  -> divide error
6  -> invalid opcode
13 -> general protection fault
14 -> page fault
~~~

Bir IRQ ise timer, klavye veya ağ kartı gibi dış donanımdan gelir. KBM, eski
PIC'i remap ederek timer IRQ0'ın CPU vector 32 olarak gelmesini sağlar.
Aynı IDT hem exception hem IRQ için kullanılır; kaynağı ve giriş stack'i farklı
olabilir.

## IDT gate yapısı

Bir x86-64 IDT girişi, handler fonksiyonunun 64 bit adresini tek alana
sığdırmak yerine üç alana böler: offset_low, offset_middle ve offset_high.
idt_set_gate bu parçaları birleştirir ve selector alanına KBM'nin code segment
selector'ı olan 0x08'i yazar.

type_attributes değeri 0x8E şu anlamı taşır:

~~~
present
Ring 0
64-bit interrupt gate
~~~

idt_init, 256 girişlik static bir tablo oluşturur. Şu an yalnızca dört fatal
exception ve timer vector 32 için gate kurar. idt_load.S içindeki lidt
talimatı, IDTR'ye bu tablonun adresini ve limitini yükler.

## CPU interrupt gelince ne yapar?

Timer örneğinde yol şöyledir:

~~~
CPU normal kod çalıştırır
  -> interrupt kabul edilir
  -> CPU dönüş bilgilerini stack'e koyar
  -> CPU IDT[32] adresine gider
  -> isr_timer assembly kodu çalışır
  -> timer_irq C fonksiyonu çağrılır
  -> isr_timer iretq ile geri döner
~~~

CPU'nun stack'e koyduğu bilgiler kesilen kodun RIP, CS ve RFLAGS değerlerini
içerir. Privilege level değişirse eski RSP ve SS de eklenir. Bazı exception'lar
ayrıca error code üretir. Bu ayrıntılar handler yazılırken önemlidir.

## interrupts.S ve ABI

isr_timer, C fonksiyonu çağırmadan önce caller-saved register'ları saklar:
RAX, RCX, RDX, RSI, RDI ve R8-R11. C çağrısı bu register'ları bozabilir.
RBX, orijinal stack adresini saklamak için kullanılır; stack daha sonra 16 byte
hizasına getirilir. System V ABI, C call öncesinde bu hizayı bekler.

timer_irq döndükten sonra register'lar ters sırayla geri yüklenir. Son satırdaki
iretq normal ret değildir. ret yalnızca normal fonksiyonun dönüş adresini alır.
iretq, CPU'nun interrupt öncesi RIP, CS ve RFLAGS durumunu stack'ten geri
yükler ve kesilen koda döner.

## Fatal exception yolu

isr_divide_error, isr_invalid_opcode, isr_general_protection_fault ve
isr_page_fault vector numarasını RDI'ye koyup ortak isr_errors yoluna atlar.
exception_fatal seri porta vector numarasını yazar ve CPU'yu durdurur. Bu
handler'lar kasıtlı olarak iretq ile dönmez; KBM'nin bugünkü politikası bu
exception'ları kurtarılamaz kabul etmektir.

## Sınırlar

Her exception için doğru error-code frame'ini ayrıştıran genel bir handler,
IST stack, kullanıcı modu exception dönüşü veya interrupt nesting desteği henüz
yoktur. Mevcut yol, erken kernel hatalarını görünür yapmak ve timer IRQ'yu
güvenli döndürmek için tasarlanmıştır.

## Kontrol noktası

1. Exception ile IRQ arasındaki fark nedir?
2. IDT[32] neden timer için kullanılır?
3. C çağrısından önce stack hizası neden önemlidir?
4. iretq neden ret ile değiştirilemez?
