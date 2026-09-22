# How to read this manual

KBM is not a product intended to compete with Linux or Windows. It is a hobby
kernel written to learn kernel development with C and x86-64 assembly. For that
reason, parts of the code are deliberately small, direct, and instructional.

## Scope of this manual

This text describes only behavior that exists in the repository. KBM receives
timer interrupts, accesses LAPIC through MMIO, builds a frame allocator from
Limine's memory map, uses a small free-list heap, and runs a framebuffer-console PS/2
keyboard shell. It does not yet have a filesystem, process model, user mode,
or custom bootloader. Those are future goals and never described as complete.

## Reading strategy

Two questions should be asked continuously while reading kernel code:

```text
Where does this data live?
Who has control at this point?
```

A value may live in a CPU register, on the stack, in normal RAM, in a page
table, or in a device register. Control may be in normal C code, the CPU's
exception entry path, an interrupt handler, Limine, or firmware. Asking these
questions is more useful than memorizing terms.

## Roles of code and manual

Source code is authoritative for actual behavior. Instead of reproducing every
function, this manual answers these questions:

- Which problem does this file solve?
- In which order do the CPU or device actions occur?
- Is an address physical, virtual, or a device address?
- Which QEMU output proves the behavior?
- What is the current limit of the code?

The checkpoint questions at the end of chapters are not exams. Their purpose is
to let the reader form a short execution story without looking at the screen.

## Prerequisites

Basic C syntax, functions, pointers, and bit operations are sufficient.
Assembly, BIOS, UEFI, page tables, and interrupts are introduced when first
needed.

## First mental model

```text
Firmware -> Limine -> KBM kmain()
                         |
                         +-> CPU tables
                         +-> output drivers
                         +-> interrupt foundation
                         +-> memory foundation
```

Read chapters in order on a first pass. When investigating a specific failure,
start with chapter 11 and then return to the relevant subsystem chapter.

## Checkpoint

1. Why is KBM's current goal not a "complete operating system"?
2. What are the two basic questions to ask while reading kernel code?
3. What is the authority relationship between the manual and source code?
