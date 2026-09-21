# PIC, PIT ve timer IRQ

Timer, kernelin ilk gerçek dış donanım olayıdır. KBM timerı yalnızca ayarlamaz;
sinyalin PIT'ten CPU'ya kadar geçtiği bütün yolu kurar.

## Yeni terimler

- **PIT:** Programmable Interval Timer; eski programlanabilir timer cihazı.
- **PIC:** Programmable Interrupt Controller; eski IRQ yönlendiricisi.
- **IRQ0:** PIT'in kullandığı legacy interrupt line.
- **EOI:** End Of Interrupt; PIC'e handler'ın bittiğini bildiren mesaj.
- **Mask:** Bir IRQ'yu geçici olarak engelleyen bit.
- **IF:** RFLAGS içindeki maskelenebilir interrupt kabul biti.

## Yolun tamamı

~~~
PIT
  -> IRQ0
PIC
  -> vector 32
LAPIC LINT0
  -> CPU interrupt
CPU
  -> IDT[32]
isr_timer
  -> timer_irq
  -> EOI
  -> iretq
~~~

Bu bölüm PIT ve PIC kısmına odaklanır. LINT0 ve LAPIC köprüsü 9. bölümde
açıklanır.

## PIC neden remap edilir?

CPU exception vector'ları 0-31 arasını kullanır. Eski PIC varsayılan olarak
IRQ0'ı düşük vector'lardan birine yönlendirebilir; bu exception'larla çakışır.
pic_remap, master PIC'i 32-39, slave PIC'i 40-47 vector aralığına taşır.

İki PIC vardır:

~~~
master PIC -> IRQ0-IRQ7
slave PIC  -> IRQ8-IRQ15
~~~

Slave PIC, master'ın IRQ2 hattı üzerinden bağlanır. pic_remap içindeki ICW1-4
yazıları iki denetleyiciyi başlatır, vector offset'lerini verir ve 8086 uyumlu
modu seçer. Ardından bütün IRQ'lar maskelenir; handler hazır olmadan hiçbir
cihazın CPU'yu kesmesi istenmez.

pic_enable_irq(0), master PIC mask register'ındaki IRQ0 bitini temizler.
Yalnızca timer hattı açılır.

## PIT nasıl ayarlanır?

PIT clock frekansı yaklaşık 1,193,182 Hz'dir. KBM 100 Hz hedefler:

~~~
divisor = 1193182 / 100 = 11931 civarı
~~~

pit_init, 0x43 command portuna 0x36 yazar; sonra divisor'ın düşük ve yüksek
byte'larını 0x40 data portuna yazar. Sonuç olarak PIT saniyede yaklaşık 100
IRQ0 üretir.

PIT ve PIC, port-mapped I/O kullanır. outb komutu RAM'e değil, x86 I/O port
alanına yazar. Küçük io_wait yazıları eski cihaz programlama sıralarında kısa
gecikme sağlar.

## CPU interrupt'ı ne zaman kabul eder?

main.c şu sırayı korur:

~~~
IDT hazırla
PIC'i remap et
PIT'i programla
IRQ0 maskesini kaldır
sti
~~~

sti, RFLAGS içindeki IF bitini açar. CPU bundan sonra maskelenebilir
interrupt'ları kabul edebilir. while döngüsündeki hlt, CPU'yu bir interrupt
gelene kadar bekletir; interrupt gelince CPU uyanır ve IDT yoluna girer.

## timer_irq ve EOI

timer_irq içindeki ticks volatile'dır çünkü normal kod bu değeri okurken
interrupt handler asenkron biçimde artırabilir. Handler her çağrıldığında
ticks'i bir artırır ve pic_send_eoi(0) çağırır.

EOI gönderilmezse PIC mevcut IRQ'nın hâlâ işlendiğini düşünür ve sonraki
interrupt'ları teslim etmeyebilir. Slave PIC'ten gelen IRQ'larda önce slave'e,
sonra master'a EOI gerekir; IRQ0 master PIC'te olduğu için yalnızca master'a
EOI gider.

## Doğrulama

main.c, ticks en az 100 olana kadar hlt ile bekler. Ardından şu satırı yazar:

~~~
KBM_PIT_TICKS: 0x0000000000000064
~~~

0x64 hexadecimal olarak 100'dür. Bu marker, PIT, PIC, LAPIC, IDT, assembly ISR,
C handler, EOI ve iretq zincirinin beraber çalıştığını kanıtlar.

## Kontrol noktası

1. PIC remap edilmezse hangi çakışma oluşur?
2. pic_enable_irq(0) hangi mask bitini değiştirir?
3. sti ve hlt birlikte neden kullanışlıdır?
4. EOI gönderilmezse ne olabilir?
