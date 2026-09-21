# KBM Manual

This manual explains KBM's current x86-64 kernel source. The order is
intentional: it starts with the machine and boot-chain model, then moves to
CPU tables, interrupts, and memory.

1. [How to read this manual](00-reading-the-manual.md)
2. [Project overview and source tree](01-project-overview-and-source-tree.md)
3. [From power-on to `kmain()`](02-from-power-on-to-kmain.md)
4. [Building and running KBM in QEMU](03-building-and-running-kbm.md)
5. [Freestanding C, output, and fatal failures](04-freestanding-c-output-and-failures.md)
6. [CPU state, GDT, and the stack](05-cpu-state-gdt-and-stack.md)
7. [IDT, exceptions, and `iretq`](06-idt-exceptions-and-iretq.md)
8. [PIC, PIT, and the timer IRQ](07-pic-pit-and-the-timer-irq.md)
9. [Physical memory, virtual memory, and paging](08-physical-memory-virtual-memory-and-paging.md)
10. [LAPIC, LINT0, and MMIO](09-lapic-lint0-and-mmio.md)
11. [The Limine memory map and the next memory layer](10-limine-memory-map-and-next-memory-layer.md)
12. [Debugging and verification with QEMU](11-debugging-and-verification.md)
13. [Current status, limits, and roadmap](12-current-status-limitations-and-roadmap.md)
14. [PMM, heap, console, keyboard, and shell](13-memory-allocator-console-keyboard-and-shell.md)
15. [Glossary](appendix-glossary.md)

Turkish edition: [Türkçe el kitabı](../tr/INDEX.md)
