# Appendix — KBM glossary

This glossary does not replace the chapters. Use it to quickly place a term encountered while reading. A term is explained where it first appears; here you find its short reference definition.

| Term | Short meaning |
| --- | --- |
| **ABI** | The contract that lets binary code cooperate: register use, stack alignment, parameters, return values, and related rules. |
| **ACPI** | Standard firmware tables that tell an operating system about power management and hardware configuration. |
| **APIC** | Advanced Programmable Interrupt Controller. The general name for the modern interrupt-routing system. |
| **BIOS** | Traditional PC firmware. It is the old boot environment and is associated with 16-bit service interrupts. |
| **bootloader** | Program that runs after firmware, loads the kernel image into memory, and transfers control to it. KBM uses Limine. |
| **CPL / ring** | CPU privilege level. Ring 0 is typical for the kernel; ring 3 for user programs. |
| **COM1 / UART** | The first classic PC serial port and the communication hardware behind it. KBM logs leave through it. |
| **CR3** | x86-64 control register 3. It contains the physical address of the active top-level page table (PML4). |
| **CS, DS, SS** | Code, data, and stack segment selector registers. In long mode their base/limit role mostly disappears, but selector and privilege rules remain. |
| **ELF** | Executable file format carrying code, data, and load information for programs such as the kernel. |
| **EOI** | End Of Interrupt. A notification to the PIC/APIC that an interrupt has been handled. |
| **exception** | An event produced by the CPU as a result of the current instruction, such as a page fault or invalid opcode. |
| **frame** | A fixed-size block of physical memory, usually 4 KiB here. It is the physical counterpart of a virtual page. |
| **framebuffer** | Pixel memory for the display. Writing pixel values changes the image. |
| **GDT** | Global Descriptor Table. Defines which code/data descriptor a segment selector refers to. |
| **HHDM** | Higher-Half Direct Map. A mapping that makes physical RAM reachable using a fixed virtual offset. |
| **IDT** | Interrupt Descriptor Table. Maps a vector number to a handler address and gate rules. |
| **IF** | Interrupt Flag in RFLAGS. When 1, maskable interrupts can be accepted by the CPU; `sti` sets it and `cli` clears it. |
| **invlpg** | x86 instruction that invalidates the TLB entry for one virtual address. Needed after a page-table change. |
| **interrupt** | A hardware- or software-caused asynchronous/synchronous transfer of control. The CPU visits a vector handler in the IDT. |
| **I/O port** | An x86 device-register address space accessed with `in`/`out` instructions. PIC, PIT, and COM1 use it. |
| **IRQ** | Interrupt Request. A hardware device's interrupt request; in classic PIC terms IRQ0 is the PIT timer. |
| **ISR** | Interrupt Service Routine. Handler code that runs when an interrupt or exception arrives. |
| **iretq** | 64-bit interrupt-return instruction. It restores RIP/CS/RFLAGS, and when required RSP/SS, from the CPU interrupt frame. |
| **kernel** | The most privileged software layer in the machine; it directly enforces rules over CPU, memory, and devices. |
| **LAPIC** | Local APIC. The local interrupt controller attached to each CPU core. |
| **LINT0** | Local Interrupt 0 input of the LAPIC. It can be used for ExtINT in the legacy PIC interrupt path. |
| **Limine** | The boot protocol and bootloader used by KBM. It supplies requested kernel information through request/response structures. |
| **long mode** | x86-64's 64-bit execution mode. It provides 64-bit registers, four-level paging, and different segment behavior. |
| **MMIO** | Memory-Mapped I/O. Device registers are accessed with loads/stores as though they were memory. The LAPIC works this way. |
| **page** | A fixed-size block in virtual address space, 4 KiB here. |
| **page table** | One of the tables that translates virtual pages to physical frames. x86-64 has PML4→PDPT→PD→PT layers. |
| **PML4 / PDPT / PD / PT** | From top to bottom, the layers of the x86-64 four-level page-table tree. Each entry points to the next table or, at the last level, a frame. |
| **PIC** | Programmable Interrupt Controller. The classic 8259-compatible interrupt controller used by KBM. |
| **PIT** | Programmable Interval Timer. The classic PC device that produces periodic timer IRQ0. |
| **PTE** | Page Table Entry. A final-level page-table entry that connects one 4 KiB virtual page to a physical frame. |
| **RFLAGS** | CPU register carrying status flags. IF is in this register. |
| **RIP / RSP** | Respectively, the next instruction address and stack pointer. |
| **selector** | A 16-bit value in a segment register such as CS/DS/SS that selects a descriptor in the GDT/LDT. |
| **serial log** | Kernel debugging output sent through COM1 to a terminal. |
| **stack** | LIFO memory area for calls, return addresses, local data, and interrupt frames. RSP tracks it on x86-64. |
| **sti / cli / hlt** | Respectively enable maskable interrupts, disable them, and stop the CPU until an interrupt arrives. |
| **TLB** | Translation Lookaside Buffer. CPU cache for recent virtual-to-physical translations. |
| **UEFI** | Modern firmware interface replacing BIOS. It provides a different early environment for disks, boot managers, and security. |
| **vector** | A 0–255 number used to select an IDT handler. For example #UD is 6; PIC IRQ0 is 32 after remapping. |
| **virtual address** | The address used by code. The CPU translates it to a physical address with page tables. |
| **physical address** | Machine address visible to RAM, MMIO, or other hardware. |

## Frequently confused pairs

| Easily confused concepts | The separating sentence |
| --- | --- |
| physical memory map / page table | The first says what physical regions are; the second says where a virtual address goes. |
| page / frame | A page is a virtual box; a frame is a physical box. |
| exception / IRQ | An exception arises from the current instruction; an IRQ is an external hardware request. |
| interrupt / ISR | The interrupt is the event; the ISR is code that handles it. |
| HHDM / allocator | HHDM provides access; an allocator provides ownership and distribution. |
| PIC / LAPIC | PIC collects legacy device interrupts; LAPIC is the modern local side that delivers them to a CPU core. |
| BIOS / bootloader | BIOS is firmware; a bootloader is software after firmware that loads the kernel. |
| GDT / IDT | GDT defines segment descriptors; IDT defines interrupt entries. |
