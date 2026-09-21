# Physical memory, virtual memory, and paging

When a kernel uses a pointer, the CPU usually does not use that value directly
as a RAM address. The pointer is a **virtual address**. The CPU consults page
tables to translate it into a physical RAM address or a device address.

## New terms

- **Physical address:** An address in the CPU's physical address space for RAM,
  firmware, or devices.
- **Virtual address:** An address used by the kernel or, later, a process.
- **Physical frame:** A 4 KiB block in physical RAM.
- **Virtual page:** A 4 KiB block in virtual address space.
- **Page table:** A table connecting a virtual page to a physical frame.
- **CR3:** A control register containing the physical base of the active PML4.
- **HHDM:** Higher-Half Direct Map; a scheme for reaching physical RAM through
  a fixed offset.
- **TLB:** A fast CPU cache of recently used address translations.

## The physical address space is not only RAM

The CPU's physical address space can contain RAM as well as firmware, ACPI
tables, PCI devices, a framebuffer, and MMIO devices such as LAPIC. Therefore
writing to a random physical address is unsafe.

The Limine memory map inventories the physical space as large ranges. For
example, USABLE means normal RAM, FRAMEBUFFER means display memory, and
RESERVED describes areas that must not be touched. A memory-map entry can cover
millions of 4 KiB frames; it is not one entry per frame.

## Pages and frames

KBM's normal paging path uses 4 KiB units:

~~~
virtual page  -> physical frame
4 KiB          -> 4 KiB
~~~

For example, a kernel virtual address may be 0xffffffff80000020. If a page
table connects its virtual page to physical frame 0x7ff41000, the final 12-bit
page offset, 0x20, is preserved:

~~~
0x7ff41000 + 0x20 = 0x7ff41020
~~~

The final 12 bits are the offset because 2^12 equals 4096 bytes.

## Why four table levels exist

The 64-bit virtual address space is very large. A single flat table with an
entry for every possible 4 KiB page could require hundreds of GiB. x86-64
splits an address and creates lower tables only for used regions:

~~~
| PML4 index | PDPT index | PD index | PT index | offset |
|    9 bits  |    9 bits  |  9 bits  |  9 bits  | 12 bits |
~~~

Each table has 512 entries. Each entry is 8 bytes, so one table is exactly 4096
bytes, the size of one physical frame.

~~~
CR3
 -> PML4
    -> PDPT
       -> PD
          -> PT
             -> physical frame
~~~

## CR3 and HHDM

CR3 contains the PML4 physical address. The CPU needs a physical root to start
translation without first using a virtual address; otherwise it would need a
translation to find the translation.

C code cannot use a physical address as a normal pointer. The HHDM response
requested from Limine in main.c provides an offset for physical **RAM**:

~~~
RAM virtual address = HHDM offset + RAM physical address
~~~

This is useful for reaching the page tables themselves because tables live in
normal RAM. A device such as LAPIC is not RAM, so HHDM alone cannot provide
LAPIC access.

## paging_translate_4k

paging_translate_4k in paging.c repeats a simplified CPU table walk in C.

1. It extracts four 9-bit indexes and a 12-bit offset from a virtual address.
2. It reads CR3 and reaches a PML4 pointer through HHDM.
3. It reads PML4, PDPT, PD, and PT entries in sequence.
4. It checks the PRESENT bit at each entry.
5. It combines the PT frame address with the original offset.

Entries contain address and flags in the same 64-bit value. ADDRESS_MASK takes
the address portion and clears low flag bits. PAGE_PRESENT is bit 0.

If the function finds PAGE_HUGE in a PDPT or PD entry, it returns false. This
is not an error; it is an explicit scope limit. The function is written to
teach and use a normal 4 KiB page chain, not yet to translate 1 GiB or 2 MiB
huge pages.

## Verification

main.c translates the virtual address of hhdm_request through
paging_translate_4k and prints the KBM_HHDM_REQUEST_PHYS marker. Debug
functions also print PML4[511], PDPT[510], PD[0], and PT[0] for the kernel's
high-half starting address.

The numeric addresses can change when QEMU or the kernel layout changes. What
matters is that the chain uses present entries and combines the final physical
frame with the correct offset.

## Checkpoint

1. What differs between a physical frame and virtual page?
2. Why does CR3 hold a physical address?
3. Which kind of physical area can HHDM reach?
4. Why is a four-level structure more efficient than one flat table?
5. Why does paging_translate_4k return false for a huge page?
