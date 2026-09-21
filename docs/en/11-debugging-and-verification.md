# 11 — Debugging and verification

Kernel debugging is harder than debugging a normal user program: there may be no file logging, the display driver may be incomplete, and one error can stop the CPU directly. This chapter uses KBM's current observable tools in a systematic way.

## 1. First find the stage where it stopped

The serial markers are not random `printf` calls. Each confirms a boundary in the boot chain. A typical sequence is:

~~~text
KBM_BOOT_OK
KBM_HHDM_READY
KBM_CR3: ...
KBM_GDT_LOADER_CALLED
KBM_IDT_LOADED
KBM_HHDM_REQUEST_PHYS: ...
KBM_PML4_511: ...
KBM_PDPT_510: ...
KBM_PD_0: ...
KBM_PT_0: ...
KBM_LAPIC_MMIO_MAPPED
KBM_PIC_REMAP_OK
KBM_PIT_TICKS: ...
KBM_MEMMAP_ENTRIES: ...
~~~

The last marker seen narrows the investigation:

| Last visible state | First place to inspect |
| --- | --- |
| No serial output | ISO/boot configuration, serial driver, reaching `kmain` |
| `BOOT_OK` but no HHDM | Limine HHDM response and null check |
| Stops just after GDT | `gdt_load.S`, selector values, stack |
| Stops after IDT | IDT gate setup, assembly stub, unexpected exception |
| Stops before LAPIC mapping | CR3 page-table walk, HHDM physical/virtual distinction |
| `PIC_REMAP_OK` but no ticks | PIC mask, PIT setup, IF, LINT0, EOI |
| Ticks but no memory map | Code after the waiting loop in `main.c` |

This table is not a final diagnosis. It is a compass for the first correct layer to examine.

## 2. Reliable run commands

Build first:

~~~sh
make all
~~~

To start with a graphical window:

~~~sh
make run
~~~

To see serial logging in the terminal, use the serial-debug target:

~~~sh
make run-serial
~~~

The options mean:

| Option | Effect |
| --- | --- |
| `-M pc` | Selects the legacy PC platform used by KBM's normal BIOS test target. |
| `-cdrom kbm.iso -boot d` | Attaches the ISO as a CD and boots from it. |
| `-m 2G` | Gives the guest 2 GiB RAM. |
| `-display none` | Does not open a graphical window. |
| `-serial stdio` | Connects COM1 output to the terminal. |
| `-monitor none` | Does not share the terminal with the QEMU monitor. |
| `-no-reboot` | Keeps failures from rebooting repeatedly and losing their logs; QEMU exits after a guest reboot request. |

`make run` is convenient for screen/framebuffer experiments and tests of the
`REBOOT` command. `make run-serial` is more practical for “which marker was
last?” questions.

## 3. The small-experiment rule

Do not change GDT, IDT, PIT, and paging code all at once. In a kernel that makes it unclear which layer broke.

A healthy loop is:

1. State one hypothesis: for example, “unmasking IRQ0 should allow the timer.”
2. Put markers immediately before and after the closest operation.
3. Build.
4. Compare serial output in QEMU.
5. Record the result, then make the next small change.

Markers are not a permanent architecture. Once a layer is reliable, remove noisy temporary output or turn it into a meaningful boot-stage log.

## 4. A safe exception experiment

If `main.c` has a commented `ud2` test, temporarily enabling it generates the CPU's invalid-opcode exception. `ud2` is an x86 instruction intentionally defined to be invalid.

The expected flow is:

~~~text
CPU executes ud2
→ vector 6 (#UD)
→ IDT[6]
→ isr_invalid_opcode
→ exception_fatal(6)
→ serial error message
→ CPU stops
~~~

This proves that the IDT does not merely look installed; it really catches an exception. Comment the test out again when finished, otherwise the timer and later stages cannot run.

Likewise, only in a controlled experiment, `int $32` can test the vector-32 path using a software interrupt. Unlike a hardware IRQ, it directly tells the CPU to visit that vector. Do not confuse this artificial test, which does not require an EOI, with a real PIT IRQ.

## 5. Common failures and their causes

### “I loaded the IDT, but it freezes after `sti`”

Possible causes:

- The IDT entry's handler address or selector is wrong.
- The gate type or present field is wrong.
- The stub damages stack balance when an IRQ arrives.
- The PIC directs an interrupt to an unexpected vector.
- An exception handler returns when it should stop, or tries to stop when it should return with `iretq`.

First separate exception from IRQ: the exception handler prints a vector number; the timer handler goes to `timer_irq`.

### “The PIT counter does not increase”

The PIT alone is insufficient. The complete route is required:

~~~text
PIT → PIC IRQ0 → LAPIC LINT0 → CPU accepts interrupt → IDT[32] → ISR → timer_irq → PIC EOI
~~~

Checklist:

- Does `pit_init()` write the correct control word and divisor?
- Is IRQ0 unmasked after `pic_remap()`?
- Was `sti` really executed? Is the IF bit in RFLAGS set?
- Is LAPIC LINT0 masked or using the wrong delivery mode instead of ExtINT?
- Does `timer_irq()` send EOI to the master PIC?
- Does the `hlt` loop run with interrupts enabled?

This project had to configure the LINT0 link explicitly. That is not a bootloader bug: a bootloader provides a reasonable starting CPU state, but the kernel must reliably take ownership of its own interrupt routing.

### “I got a page fault or it stops when accessing the LAPIC”

The most common error is treating a physical address as a virtual address. The LAPIC physical address `0xFEE00000` cannot be used directly as a C pointer; that physical page must first be mapped to a virtual page through a PTE.

Verify one item at a time:

- Do you convert the PML4 physical address read from CR3 to an accessible pointer through the HHDM?
- Is the result of `paging_translate_4k` truly a physical address?
- Did you create empty PML4/PDPT/PD/PT entries with suitable present/writable bits?
- Does the PTE contain the LAPIC physical frame and cache-control bits?
- Do you access it after `invlpg`?

## 6. Think across the assembly/C boundary

Entering a C function needs a normal calling convention; a hardware interrupt arrives with a different CPU-created stack frame. That is why `interrupts.S` is not “unnecessary assembly.” It is a bridge:

~~~text
CPU interrupt frame
→ register save
→ 16-byte stack alignment
→ C handler call
→ register restore
→ iretq
~~~

When an ISR fails, first ask: “Is the problem in the C logic, or in the stack contract before entering/after leaving C?” This distinction saves a great deal of time.

## 7. GDB: a next-level tool

When serial logging is insufficient, QEMU can be paused for GDB. A typical approach starts QEMU with a debug port and a stopped CPU:

~~~sh
qemu-system-x86_64 ... -s -S
~~~

From another terminal:

~~~sh
gdb kernel/bin/kernel
(gdb) target remote :1234
(gdb) break kmain
(gdb) continue
~~~

`-S` stops the CPU until GDB continues it; `-s` opens a GDB stub on TCP port 1234 by default. This is useful, but logs and small experiments should be your first debugging tools. GDB can initially feel heavy because of assembly, optimization, and symbol/address details.

## 8. Lesson from the LAPIC/LINT0 debugging diary

In this project PIT code, PIC remapping, and the IDT looked functional, yet no tick arrived. The problem was not “the PIT is QEMU-specific and broken.” The PIC's legacy interrupt output reaches the CPU through the LAPIC's **LINT0** input, and LINT0 was masked in ExtINT mode.

The solution's logic was:

1. Map the LAPIC physical register page at a dedicated virtual address.
2. Read the LINT0 Local Vector Table register.
3. Set its delivery mode to ExtINT.
4. Clear its mask bit.
5. Once PIC IRQ0 reaches IDT vector 32, `timer_irq` increments the counter.

The main gain is not memorizing individual register bits. It is learning this method: **follow a hardware signal through every link, from source to handler.**

## Checkpoint

- The last boot marker tells you which source layer to inspect first.
- `ud2` is a controlled exception test for vector 6.
- A tick failure may be in any link from PIT to IDT.
- GDB is a next-level tool; serial output is the first observation tool.
