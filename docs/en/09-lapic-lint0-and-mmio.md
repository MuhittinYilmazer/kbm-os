# LAPIC, LINT0, and MMIO

At first, KBM programmed the PIT and unmasked PIC IRQ0 but received no ticks.
The problem was not PIT or IDT: the legacy PIC signal reached LAPIC, but the
LAPIC LINT0 input was masked.

This chapter explains the problem and solution through the complete route.

## New terms

- **APIC:** The Advanced Programmable Interrupt Controller family.
- **LAPIC:** Local APIC; an interrupt controller close to a CPU core.
- **LINT0:** Local Interrupt input 0; the line where legacy PIC output enters
  LAPIC.
- **ExtINT:** The LINT0 delivery mode that accepts a legacy PIC interrupt.
- **MMIO:** Memory-Mapped I/O; accessing device registers through memory-like
  addresses.
- **PCD/PWT:** Page-table cache-behavior bits.
- **invlpg:** An x86 instruction that invalidates one virtual address in TLB.

## Where the APIC layer sits

The current legacy timer route is:

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

PIC is an older interrupt controller. LAPIC is the local interrupt controller
present in modern x86 CPUs. In a multicore system, each core has its own LAPIC.
The I/O APIC is another, more modern component for external device interrupts;
KBM does not use I/O APIC yet.

During QEMU investigation, PIC had IRQ0 pending while LAPIC LINT0 was masked
ExtINT. Therefore execution never entered isr_timer.

## Why LAPIC needs a special mapping

The LAPIC physical base is:

~~~
0xFEE00000
~~~

This is not normal RAM; it is a physical MMIO range containing device
registers. HHDM maps physical RAM into virtual addresses. Therefore adding the
HHDM offset to the LAPIC physical base does not reliably create an LAPIC
pointer.

KBM chooses its own LAPIC virtual address:

~~~
LAPIC virtual base:  0xFFFFFFFFC0000000
LAPIC physical base: 0x00000000FEE00000
~~~

The intended result is:

~~~
If the kernel accesses 0xFFFFFFFFC0000000
  -> CPU walks page tables
  -> the result is 0xFEE00000
  -> the operation reaches the LAPIC device
~~~

## New page tables

The two static, 4096-byte-aligned arrays in lapic.c begin as ordinary RAM data:

~~~
lapic_page_directory[512]
lapic_page_table[512]
~~~

Each has 512 eight-byte entries, exactly one 4 KiB page-table size.
lapic_enable_legacy_pic first finds the physical addresses of these arrays
through paging_translate_4k. This is necessary because a page-table entry needs
a physical address for the next table.

The existing PML4 is found through CR3 and HHDM. The kernel's high-half area
already lies under PML4[511], so that entry leads to an existing Limine PDPT.
KBM uses the PDPT[511] entry that should be empty for the selected LAPIC
virtual address:

~~~
PML4[511]                  -> Limine's existing PDPT
PDPT[511]                  -> lapic_page_directory physical address
lapic_page_directory[0]    -> lapic_page_table physical address
lapic_page_table[0]        -> 0xFEE00000 + page flags
~~~

If PDPT[511] is already present, the function returns false. This is a simple
safety check against overwriting an existing mapping in the selected virtual
region.

## MMIO page flags and TLB

Intermediate table entries use PRESENT | WRITABLE. The final LAPIC PTE adds
PAGE_WRITE_THROUGH and PAGE_CACHE_DISABLE. The goal is to avoid treating device
registers like ordinary cacheable RAM.

After the new translation is written, KBM calls:

~~~
invlpg(LAPIC_VIRTUAL_BASE)
~~~

This tells the CPU to forget old TLB information for the virtual address. Even
if the CPU has not used the LAPIC address before, calling invlpg after changing
a mapping is a safe and instructive habit.

The lapic pointer is volatile uint32_t. Reads and writes to a device register
are observable hardware operations, so the compiler must not remove them as if
they were ordinary RAM accesses.

## LINT0 configuration

The LAPIC LINT0 register has offset 0x350. Because the pointer is uint32_t,
the code reaches that 32-bit register with lapic[0x350 / 4].

It reads the current value and changes only the needed fields:

~~~
bits 8-10 -> delivery mode; set to 111: ExtINT
bit 16    -> mask; clear to 0: unmasked
~~~

This tells LAPIC:

~~~
Accept interrupts arriving from the legacy PIC through LINT0.
~~~

This code does not program the PIT or unmask PIC IRQ0. pit_init programs the
PIT and pic_enable_irq(0) opens the PIC mask. LINT0 opens the missing bridge
between those legacy devices and the CPU.

## Proof and limits

The KBM_PIT_TICKS: 0x64 output means 100 timer interrupts were handled. It
proves these components work together:

~~~
PIT -> PIC -> LINT0 -> LAPIC -> CPU -> IDT -> ISR -> timer_irq -> EOI -> iretq
~~~

This LAPIC code is a bootstrap solution. It uses a fixed LAPIC physical base
and a fixed selected virtual address. There is no I/O APIC, x2APIC, LAPIC
timer, SMP support, or general-purpose MMIO mapper yet.

## Checkpoint

1. Why is HHDM alone insufficient for LAPIC access?
2. Why does lapic_page_directory begin as an ordinary C array?
3. Why do page-table entries store physical rather than virtual addresses?
4. What do the ExtINT and unmasked settings in LINT0 mean?
5. Which complete chain does KBM_PIT_TICKS prove?
