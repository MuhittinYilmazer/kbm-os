# KBM Manual Plan

## Purpose

This plan defines the structure of the KBM manual before the manual itself is
written. The manual is for a reader who knows basic C but may not know x86-64,
firmware, bootloaders, interrupts, or virtual memory. Its first audience is a
future version of the KBM author; its second audience is another beginner who
wants to understand the current source tree.

The completed manual will exist in two parallel editions:

```text
docs/tr/  Turkish manual
docs/en/  English manual
```

The chapter order and technical claims must match between the two editions.
The English edition is a translation of the Turkish edition, not a shorter
overview.

## Editorial rules

Each chapter follows this order when it is useful:

1. **Question solved** — the concrete problem that motivated the subsystem.
2. **New terms** — terms are defined before they are used in depth.
3. **Mental model** — a small diagram or story establishes the relationship
   between hardware, CPU, bootloader, and kernel code.
4. **KBM implementation** — the relevant current files and the responsibility
   of each function.
5. **Execution path** — how control or data moves at runtime.
6. **Verification** — a build, QEMU marker, or deliberate test that proves the
   described behavior.
7. **Current limits** — behavior that is deliberately absent or only a
   bootstrap implementation.
8. **Checkpoint** — short questions a reader should be able to answer without
   looking at the code.

The source code remains the authoritative implementation. The manual explains
why code exists and traces important excerpts; it does not duplicate whole
files without a teaching reason. Current behavior and planned work are always
labelled separately.

## Chapter map

| No. | File name | Main question | Current source evidence |
| --- | --- | --- | --- |
| 00 | `00-reading-the-manual.md` | What does KBM aim to teach and how should this manual be read? | whole repository |
| 01 | `01-project-overview-and-source-tree.md` | What exists in KBM today and where does each responsibility live? | `kernel/src/` |
| 02 | `02-from-power-on-to-kmain.md` | How do x86 reset, BIOS/UEFI, Limine, long mode, and `kmain` fit together? | `limine.conf`, linker script, `main.c` |
| 03 | `03-building-and-running-kbm.md` | How is the ISO built and how is it observed in QEMU? | root `GNUmakefile`, kernel `GNUmakefile` |
| 04 | `04-freestanding-c-output-and-failures.md` | What changes when C runs without a hosted standard library? | `memory.c`, serial, framebuffer, panic |
| 05 | `05-cpu-state-gdt-and-stack.md` | What are registers, stack, segments, GDT, selectors, and long-mode rules? | `cpu.h`, `gdt.c`, `gdt_load.S` |
| 06 | `06-idt-exceptions-and-iretq.md` | How does the CPU enter and return from an exception or interrupt handler? | `idt.*`, `interrupts.S`, exception code |
| 07 | `07-pic-pit-and-the-timer-irq.md` | How does PIT IRQ0 become IDT vector 32 and increase `ticks`? | `pic.*`, `pit.*`, timer code |
| 08 | `08-physical-memory-virtual-memory-and-paging.md` | What are physical frames, virtual pages, CR3, HHDM, and four-level paging? | `paging.*`, linker script, `main.c` |
| 09 | `09-lapic-lint0-and-mmio.md` | Why was the timer blocked and how does KBM map/configure LAPIC LINT0? | `lapic.*`, paging, PIC/PIT |
| 10 | `10-limine-memory-map-and-next-memory-layer.md` | What does the Limine memory map report and what must happen before an allocator exists? | `main.c`, Limine definitions |
| 11 | `11-debugging-and-verification.md` | How are serial markers, QEMU, deliberate faults, and tests used to investigate KBM? | serial driver, `main.c`, build scripts |
| 12 | `12-current-status-limitations-and-roadmap.md` | What is finished, what is intentionally small, and what should come next? | whole repository |
| 13 | `13-memory-allocator-console-keyboard-and-shell.md` | How do PMM, heap, console, IRQ1 keyboard input, and the shell connect? | `pmm.*`, `heap.*`, console, keyboard, shell |
| A | `appendix-glossary.md` | What do recurring terms mean in one or two precise sentences? | all chapters |

## Navigation and language layout

Each language directory contains the complete chapter map above plus an
`INDEX.md` linking the chapters in reading order. The root `README.md` becomes
a short project landing page with links to both manuals, build commands, and a
brief current-feature list. It does not replace the detailed manuals.

The Turkish edition introduces English technical terms on first use, for
example `fiziksel frame (physical frame)`. The English edition keeps the same
diagrams, examples, source-file references, verification markers, limits, and
checkpoint questions.

## Scope boundary

The manual documents the current KBM implementation, including its learning
experiments. KBM currently has a basic frame allocator, free-list heap,
framebuffer console, PS/2 keyboard input, and shell. It must not claim a
filesystem, user mode, process scheduler, I/O APIC support, SMP, or a custom
bootloader.
Those topics may appear only as planned work with their prerequisites stated.

## Completion checks

Before the manual is considered complete:

- every planned Turkish chapter exists;
- every planned English chapter exists with equivalent content;
- all code references name files that currently exist;
- every current feature claim is supported by source and, where applicable,
  an observed QEMU marker;
- planned features are clearly separated from implemented features;
- root README links to both indexes;
- Markdown links are checked locally for missing targets.
