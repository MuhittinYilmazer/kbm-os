# Project overview and source tree

KBM is a freestanding x86-64 kernel loaded by Limine. Freestanding means that
the C program does not depend on Linux, glibc, or a normal C runtime. The
kernel must provide its own startup environment, memory routines, and hardware
access.

## Current behavior summary

KBM currently does the following:

~~~
Boots through Limine.
Writes logs to the COM1 serial port.
Contains basic framebuffer drawing functions.
Loads its own GDT and IDT.
Reports selected CPU exceptions as fatal errors.
Programs the PIT timer at 100 Hz.
Receives timer IRQs through PIC, LAPIC LINT0, and the IDT.
Translates virtual addresses to physical addresses.
Creates an LAPIC MMIO mapping.
Prints the Limine physical memory map.
~~~

## Source tree

~~~
kernel/src/
├── main.c                 Post-boot initialization order
├── memory.c/.h            Freestanding memory routines
├── arch/x86_64/           x86-64-specific CPU and interrupt code
├── drivers/               COM1, framebuffer, and PIT drivers
└── kernel/                Panic, exception, timer, and font logic
~~~

Files under arch/x86_64 would not work unchanged on another processor
architecture. GDT, IDT, PIC, and LAPIC belong to the x86 world. In contrast,
the ticks counter in kernel/timer.c can be viewed as a more portable kernel
layer; the interrupt entry path that calls it is architecture-specific.

## Responsibility boundaries

| File or group | Responsibility |
| --- | --- |
| main.c | Initialization order, Limine response checks, boot logs |
| serial.* | COM1 output for early debugging |
| gdt.*, idt.*, interrupts.S | CPU segment and interrupt tables |
| pic.*, pit.*, lapic.* | Hardware layers in the timer interrupt chain |
| paging.* | Virtual-to-physical translation through existing 4 KiB mappings |
| memory.* | Basic C memory routines the compiler may require |
| panic.*, exception.* | Reporting and stopping when safe continuation is impossible |

## Implementation and experiment code

lapic_debug_pml4_511, lapic_debug_pdpt_510, lapic_debug_pd_0, and
lapic_debug_pt_0 are instructional debug functions added to observe a page
table walk. They are not a general-purpose paging interface for normal kernel
work. By contrast, paging_translate_4k is used on the boot path to find the
physical addresses of the LAPIC table arrays.

## Checkpoint

1. Why does main.c not implement every hardware driver directly?
2. Which directory isolates x86-64-specific code?
3. What is the difference between a debug function and a lasting kernel API?
