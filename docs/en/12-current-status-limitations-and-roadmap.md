# 12 — Current state, limits, and roadmap

KBM is not yet a “working general-purpose operating system.” That is not a negative judgment; it is the accurate engineering description. KBM is a learning-focused kernel beginning that has actually executed the fundamental links of x86-64 kernel bring-up.

## 1. What genuinely works now

The following items exist in the source and can be verified in QEMU:

| Layer | KBM's state |
| --- | --- |
| Boot | Limine loads the kernel ELF and transfers to `kmain`. |
| Serial output | It writes boot and error markers through COM1. |
| Display and console | 8×8 framebuffer text, wrapping, scrolling, clear, and backspace work. |
| Core C routines | `memcpy`, `memset`, `memmove`, and `memcmp` are supplied by the kernel. |
| CPU setup | A minimal GDT is loaded; code/data selectors are installed. |
| Exception foundation | The IDT contains selected exception vectors and fatal handlers. |
| Hardware interrupt foundation | There are IDT entries, assembly ISR stubs, and 8259 PIC remap/mask/EOI operations. |
| Timer | The PIT generates IRQ0 at roughly 100 Hz; the timer counter increases. |
| LAPIC | The LAPIC MMIO page is mapped at a dedicated virtual address; the legacy PIC interrupt route is enabled through LINT0. |
| Paging inspection | A 4 KiB page-table walk starting at CR3 and virtual-to-physical translation exist. |
| Memory discovery | The Limine physical memory map is printed over serial. |
| PMM | Two-bitmap 4 KiB frame allocation/free and ownership checks work. |
| Heap | A small 16-byte-aligned PMM-backed free-list allocator supports `kmalloc`, `kfree`, and adjacent-free-block merging. |
| Keyboard and shell | PS/2 IRQ1, a Turkish-Q subset, scrolling framebuffer shell, uptime display, and reboot command work. |

“Exists” is not the same as “production quality.” Exception support is limited to a few vectors, the timer only counts ticks, and framebuffer code is not yet a terminal or GUI.

## 2. What does not exist yet

The following are expected, deliberate next-work items:

- General-purpose page allocate and map/unmap API
- Filesystem, disk driver, VFS
- User mode, processes, scheduler, or syscall ABI
- ELF user-program loading
- Networking
- Multicore (SMP) and AP startup
- Security boundaries, user address spaces, and access control
- Persistent test infrastructure and debugger integration

This is not a list of things you must all do. Choosing the scope is part of succeeding at a hobby kernel.

## 3. Roadmap: small, verifiable milestones

### Milestone A — Physical memory management (complete for v0.1 scope)

Goal: obtain 4 KiB frames from safe usable RAM.

1. Inspect `USABLE` ranges in the Limine memory map.
2. Write a temporary physical bump allocator.
3. Show several allocated frames in the serial log.
4. Add allocation and freeing using a bitmap allocator.
5. Reserve the kernel, bootloader, framebuffer, and the bitmap's own storage.

At the end of this stage, code answers “which physical RAM can the kernel use?”

### Milestone B — Kernel heap and dynamic structures (basic free-list version complete)

Goal: stop forcing the kernel to live only with static arrays.

1. Obtain frames from the physical allocator.
2. Map frames into a selected kernel virtual region.
3. Build the first simple heap.
4. Verify alignment, frame growth, and accounting through small tests.
5. Add a free-list, `kfree`, and merging for adjacent free blocks.

Keeping the initial heap simple is wise. At this stage the goal is not maximum performance; it is understanding ownership and failure behavior.

### Milestone C — Human interaction (basic version complete)

Goal: give the user meaningful feedback without a serial terminal.

1. Draw text, wrapping, and scrolling on the framebuffer.
2. Accept PS/2 keyboard input through IRQ1 with a Turkish-Q subset.
3. Build a small kernel console with writing, backspace, and a command line.
4. Add diagnostic commands including `HELP`, `MEM`, `TICKS`, `UPTIME`, and `REBOOT`.

Here KBM starts to visibly feel like a “system.” It still does not need to be a user-mode OS.

### Milestone D — Storage and a simple filesystem

Goal: work with persistent data.

1. Learn the block-device abstraction and make a read API.
2. Start with a RAM disk; do not jump directly to a real disk.
3. Implement a tiny custom read-only filesystem or simple FAT reading.
4. List files and show file contents.

This stage combines device drivers, caching, error handling, and data formats. Keeping its scope small is especially important.

### Milestone E — Processes and user mode (optional, advanced)

Goal: truly separate the kernel from applications.

1. Extend GDT/IDT and privilege rules for ring 3.
2. Build separate user page tables.
3. Design a syscall entry.
4. Write a minimal scheduler and context switch.
5. Load a small user program.

It is highly educational but depends on the earlier milestones being solid. It is not mandatory for KBM's learning goal.

## 4. Why the current order makes sense

The order comes from dependencies:

~~~text
memory map
  → physical frames
    → mapped kernel memory
      → heap
        → dynamic drivers / console / filesystem structures
          → (if desired) processes and user mode
~~~

Timer and interrupt infrastructure arrived earlier because they will eventually underpin a scheduler, but having a timer does not mean a scheduler exists. Likewise, having a GDT does not mean ring-3 user mode exists.

## 5. How should success be measured?

KBM's progress should not be measured by line count or similarity to Linux. Better measures are:

- Can you explain which problem a feature solves?
- When a failure occurs, can you narrow the nearest layer systematically?
- Can you verify behavior through a small QEMU experiment?
- Are ownership boundaries clear in code: “who owns this physical frame?” and “where did this interrupt come from?”
- Can you return to the code a week later using comments and this manual?

The most valuable result is not the booting ISO. It is your ability to build the causal chain between CPU, memory, and devices.

## 6. Suggested work rhythm

Choose only one small target per session:

~~~text
Today: find the first aligned frame in USABLE ranges.
Then: print that frame address.
Then: verify that the second frame is different and aligned.
~~~

For every target, write these four lines in a notes file:

1. Which assumption am I testing?
2. Which source files do this work?
3. What evidence do I expect in QEMU?
4. If it fails, which layer will I inspect first?

This rhythm turns complex kernel work into manageable experiments.

## 7. How should this project be described?

KBM is not intended to compete with Linux or Windows. An honest description is:

> A learning-focused x86-64 hobby kernel that boots through Limine and contains GDT, IDT, PIC/PIT timer, LAPIC MMIO mapping, basic paging inspection, and memory-map discovery.

It neither diminishes the work already done nor pretends absent features exist. That is the basis of good technical communication.

## Final checkpoint

The most natural next heap task is returning a completely unused heap frame to
PMM. Before starting it, reread Chapters 8, 9, and 10; especially internalize
this sentence:

> Paging establishes whether an address is reachable; an allocator establishes ownership of the RAM behind that address.
