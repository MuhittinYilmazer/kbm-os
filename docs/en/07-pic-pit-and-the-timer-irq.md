# PIC, PIT, and the timer IRQ

The timer is the kernel's first real external hardware event. KBM does not only
program it; it establishes the complete route from PIT to CPU.

## New terms

- **PIT:** Programmable Interval Timer; an older programmable timer device.
- **PIC:** Programmable Interrupt Controller; an older IRQ router.
- **IRQ0:** The legacy interrupt line used by the PIT.
- **EOI:** End Of Interrupt; a message telling the PIC that handling ended.
- **Mask:** A bit that temporarily blocks an IRQ.
- **IF:** The maskable-interrupt enable bit in RFLAGS.

## The complete route

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

This chapter focuses on the PIT and PIC portions. The LINT0 and LAPIC bridge is
explained in chapter 9.

## Why the PIC is remapped

CPU exception vectors occupy 0 through 31. A legacy PIC can route IRQ0 to a
low default vector, which would collide with exceptions. pic_remap moves the
master PIC to vectors 32-39 and the slave PIC to 40-47.

There are two PICs:

~~~
master PIC -> IRQ0-IRQ7
slave PIC  -> IRQ8-IRQ15
~~~

The slave connects through the master's IRQ2 line. The ICW1-4 writes in
pic_remap initialize both controllers, provide their vector offsets, and choose
8086-compatible mode. All IRQs are then masked so no device can interrupt the
CPU before its handler is ready.

pic_enable_irq(0) clears the IRQ0 bit in the master PIC mask register. Only the
timer line is opened.

## How the PIT is programmed

The PIT clock frequency is about 1,193,182 Hz. KBM targets 100 Hz:

~~~
divisor = 1193182 / 100 = about 11931
~~~

pit_init writes 0x36 to command port 0x43, then writes the low and high bytes
of the divisor to data port 0x40. The result is roughly 100 IRQ0 events per
second.

PIT and PIC use port-mapped I/O. The outb instruction writes to the x86 I/O
port address space rather than RAM. Small io_wait writes provide short delays
in old device-programming sequences.

## When the CPU accepts an interrupt

main.c preserves this order:

~~~
prepare IDT
remap PIC
program PIT
unmask IRQ0
sti
~~~

sti enables the IF bit in RFLAGS. The CPU can then accept maskable interrupts.
The hlt in the while loop waits until an interrupt arrives; the CPU wakes and
enters the IDT path when one does.

## timer_irq and EOI

ticks in timer_irq is volatile because the interrupt handler can increment it
asynchronously while normal code reads it. On each call the handler increments
ticks and calls pic_send_eoi(0).

Without EOI, a PIC can consider the current IRQ still in service and may not
deliver later interrupts. IRQs from the slave need EOI first at the slave and
then at the master; IRQ0 belongs to the master, so it needs only a master EOI.

## Verification

main.c waits with hlt until ticks reaches at least 100, then prints:

~~~
KBM_PIT_TICKS: 0x0000000000000064
~~~

0x64 is hexadecimal 100. This marker proves that PIT, PIC, LAPIC, IDT, the
assembly ISR, C handler, EOI, and iretq work together.

## Checkpoint

1. What conflict occurs if the PIC is not remapped?
2. Which mask bit does pic_enable_irq(0) change?
3. Why are sti and hlt useful together?
4. What can happen if EOI is not sent?
