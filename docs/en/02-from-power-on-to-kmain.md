# From power-on to kmain()

This chapter explains the layers a computer passes through before KBM runs. A
kernel is not the first code that executes when power is applied.

## New terms

- **Firmware:** The first startup software stored in motherboard flash.
- **BIOS:** The older PC firmware standard.
- **UEFI:** The modern firmware standard.
- **Bootloader:** A short-lived program that finds the kernel, loads it into
  RAM, and prepares an executable environment.
- **Long mode:** The 64-bit operating mode of an x86-64 processor.
- **Limine:** The bootloader used by KBM.

## The boot story

~~~
Power is applied
  -> CPU resets
  -> firmware executes
  -> firmware finds a bootloader
  -> Limine loads the kernel ELF into RAM
  -> Limine prepares page tables and boot information
  -> Limine jumps to kmain()
~~~

For x86 compatibility, a CPU begins after reset in an old 16-bit startup
environment. On the BIOS boot path, Limine comes from that environment and
prepares 64-bit long mode and paging for the kernel. On an x86-64 UEFI path,
UEFI may already execute a bootloader in a 64-bit environment; Limine still
prepares the memory layout and response structures expected by the kernel.

## BIOS and UEFI

Classic BIOS runs initial boot code from a bootable disk or CD. Older BIOS
programs could ask firmware services to do work such as video output through
int 0x10. Those services belong to the 16-bit real-mode world.

UEFI normally runs an EFI executable such as BOOTX64.EFI from an EFI System
Partition on a GPT disk. UEFI can supply more modern boot information such as
a memory map and framebuffer. If Secure Boot is enabled, firmware can verify
signatures in the boot chain.

The KBM ISO contains Limine components for both BIOS and UEFI boot. The normal
root make run target uses QEMU's default BIOS path because it does not supply a
separate UEFI firmware. make run-uefi uses an OVMF-based UEFI firmware when it
has been obtained.

## Contract between Limine and KBM

The .limine_requests section in main.c contains the information requested from
Limine:

~~~
framebuffer request -> framebuffer information for pixel output
HHDM request        -> an offset for reaching physical RAM
memmap request      -> an inventory of physical address ranges
~~~

The used and section attributes prevent the compiler from discarding these
structures and place them in the correct section. The linker script's
.limine_requests section lets Limine find them.

## Why the kernel begins at a high address

kernel/linker-scripts/x86_64.lds starts kernel sections at the virtual address
0xffffffff80000000. This is a high-half kernel layout. Lower virtual addresses
can later be reserved for user programs while the kernel lives in a stable high
region. This is not a physical RAM address. Limine's page tables connect those
high virtual addresses to the physical pages containing the kernel.

## After kmain() begins

Limine transfers control to KBM. KBM first initializes the serial port, checks
Limine response pointers, and then loads its own GDT and IDT. From that point,
an increasing part of CPU exception and interrupt behavior is managed by KBM's
own code.

## Checkpoint

1. What is the responsibility difference between firmware and a bootloader?
2. Why does long mode matter to the kernel?
3. How do HHDM and memory-map information reach KBM?
4. Why is 0xffffffff80000000 not a physical RAM address?
