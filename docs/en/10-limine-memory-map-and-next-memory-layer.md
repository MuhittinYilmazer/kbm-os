# 10 — The Limine memory map and the next memory layer

This chapter explains what the raw memory-map lines printed at the end of `main.c` mean, and which problem the memory manager that has not yet been written will solve.

## 1. Two different “maps”

First separate two very similar-looking ideas.

- The **physical memory map** describes the purpose of physical-address ranges in the machine: usable RAM, bootloader-owned memory, framebuffer memory, and so on.
- A **page mapping** tells the CPU which physical address a virtual address can translate to.

Limine supplies the first; CR3 and the page tables determine the second. A physical range being “usable” does not mean it is automatically reachable from every virtual address. Conversely, a mapped virtual address may refer to an MMIO device register rather than RAM. The LAPIC chapter was a live example.

## 2. The memory map supplied by Limine

`main.c` asks Limine for a map using `limine_memmap_request`. The response has two important fields:

~~~c
memmap_request.response->entry_count
memmap_request.response->entries[i]
~~~

Every entry roughly carries three pieces of information:

| Field | Meaning |
| --- | --- |
| `base` | Physical start address |
| `length` | Length of this range in bytes |
| `type` | The use category of the range |

For now the kernel only writes these entries to the serial port. That is the right start before an allocator exists: obtain trustworthy facts about the available resource before managing it.

With QEMU started using `-m 2G`, seeing one large `USABLE` range is normal. You will also see low reserved areas, areas held for the kernel or bootloader, and special non-normal-RAM ranges such as the framebuffer. Exact entry counts and addresses vary with the QEMU version, RAM size, and boot method; do not memorize them.

## 3. Types: which memory is immediately usable?

Limine's types are defined in `limine.h`. A practical interpretation for this kernel is:

| Type | Meaning to an initial allocator |
| --- | --- |
| `USABLE` | Candidate free physical RAM. The first physical allocator works here. |
| `BOOTLOADER_RECLAIMABLE` | May be reclaimed later once bootloader work is over; be conservative initially. |
| `EXECUTABLE_AND_MODULES` | Holds the kernel and modules; you must not overwrite your own code. |
| `FRAMEBUFFER` | Screen memory; it is not handed out as normal general-purpose RAM. |
| `RESERVED`, `ACPI_*`, `BAD_MEMORY` | Ranges with special ownership or meaning; a normal allocator does not call them free. |

The first safe policy is simple: allocate only from `USABLE` entries and treat everything else as untouchable. It may waste some memory, but avoids overwriting an unknown address.

## 4. “Why does 16 GB of RAM appear as only a few records?”

A memory map does not list RAM byte by byte, nor DIMM by DIMM. A large **range** with the same property is one record.

For example, a usable 2 GiB-sized range could read:

~~~text
base   = 0x00100000
length = 0x7fe3d000
type   = USABLE
~~~

This does not say that the computer has one RAM chip. It says that the bytes in that physical-address interval may be used as general-purpose RAM by the kernel. An allocator later divides this large interval into 4 KiB physical **frames**.

A **page** is a 4 KiB box in virtual address space. The corresponding physical 4 KiB box is often called a **frame**. They can have the same size, but one is an addressing view and the other is an actual RAM resource.

## 5. The next three allocators and their roles

“Bump allocator, bitmap allocator, heap allocator” are three names, but they are not three competing tools at the same layer.

### 5.1 Physical bump allocator

It stores the beginning of a usable range. When asked for a frame, it returns the next aligned 4 KiB and advances its pointer.

~~~text
start -> [given][given][next][free][free]...
                           ^
~~~

Its advantage is that it is very short and ideal for bringing up initial page tables or data structures.

Its missing feature is freeing: it cannot reuse a frame after it is released. It is therefore not a final kernel memory manager.

### 5.2 Physical bitmap allocator

It keeps one bit per physical frame:

~~~text
frame:  0 1 2 3 4 5 6 7
bit:    1 1 0 1 0 0 1 0
        ^ in use          0 = free, 1 = reserved (by our chosen convention)
~~~

This lets you reserve a particular frame and later free it again. It is more suitable for a real memory manager. The difficulty is placing the bitmap itself in safe RAM and correctly marking kernel, bootloader, and MMIO ranges at startup.

### 5.3 Kernel heap allocator

A heap is for requests such as `kmalloc` asking for small blocks of 24 bytes, 100 bytes, or a structure size. A heap does not hand out arbitrary bytes directly from physical RAM. It first asks the physical allocator for frames, maps those frames into kernel virtual space, then subdivides them into small blocks.

~~~text
physical allocator -> 4 KiB frame
page mapper        -> reachable page in kernel virtual address space
heap allocator     -> 24/100/... byte block to a caller
~~~

Starting directly with a heap is technically possible, but then you must secretly solve the lower two layers anyway. For learning, bump → bitmap → heap is a sensible order because each exposes the problem the next layer depends on.

## 6. What this project does not have yet

KBM now has a two-bitmap PMM that distributes and frees physical frames plus a
small 16-byte-aligned free-list heap. Chapter 13 describes its implementation,
tests, and limits. HHDM access still does not mean the kernel has **claimed**
RAM; an allocator tracks which frame may safely be used by whom.

The distinction matters:

~~~text
HHDM:       “I can access this physical address.”
allocator:  “This physical frame belongs to me and is safe to use.”
~~~

## 7. Implementation plan

Build the next memory work in this order:

1. Scan `USABLE` entries and choose a suitable first 4 KiB-aligned range.
2. Take care not to overlap the kernel, Limine structures, or critical temporary data.
3. Write a tiny physical bump allocator that only returns frames.
4. Allocate a few frames and print their physical addresses over serial.
5. Then calculate how many frames and bits a bitmap needs.
6. Place the bitmap in a safe usable range and mark frames that must not be used.
7. Do not write a heap before the physical allocator is trustworthy.

The first version's goal is not “use all RAM efficiently.” It is **never distribute the wrong memory**.

## Checkpoint

After this chapter you should be able to say in your own words:

- The Limine memory map is not a page table.
- `USABLE` is a candidate pool; not every entry is immediately free.
- HHDM provides access; an allocator tracks ownership.
- A bump allocator returns frames quickly but cannot reclaim them.
- A bitmap tracks physical frames; a heap manages smaller kernel allocations.
