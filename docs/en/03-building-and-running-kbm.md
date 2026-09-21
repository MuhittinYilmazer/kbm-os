# Building and running KBM in QEMU

KBM is compiled as a freestanding ELF kernel, placed in a bootable ISO with
Limine files, and run in QEMU.

## New terms

- **Toolchain:** Build tools such as a compiler, assembler, and linker.
- **ELF:** A common executable format on x86-64 Unix-like systems.
- **Linker:** A tool that combines object files and establishes address layout.
- **ISO:** A boot image similar to a CD-ROM image.
- **QEMU:** An emulator and virtualizer that can model hardware.

## Normal build

At the repository root:

~~~
make all
~~~

The command first produces the kernel ELF through the Makefile under kernel.
The root Makefile then does the following:

~~~
copies kernel/bin/kernel into an ISO tree
adds Limine BIOS and UEFI files
creates kbm.iso through xorriso
installs BIOS boot information through limine bios-install
~~~

The final product is kbm.iso at the repository root.

## Running through QEMU

For a graphical window:

~~~
make run
~~~

For early kernel logs in the terminal, it is more useful to connect the serial
port to standard input and output:

~~~
qemu-system-x86_64 -M q35 -cdrom kbm.iso -boot d -m 2G \
  -display none -serial stdio -monitor none -no-reboot
~~~

Here -m 2G gives the QEMU virtual machine 2 GiB of RAM. It does not describe
the amount of physical RAM in the host computer.

## Build layers

kernel/GNUmakefile finds .c and .S files under the full src tree. C files are
built with freestanding flags:

~~~
-ffreestanding  -> do not assume a hosted C environment
-nostdinc       -> do not use host standard include directories
-m64            -> generate x86-64 code
-mno-red-zone   -> choose an ABI behavior safe for an interrupting kernel
~~~

The linker script makes kmain the entry point and establishes the high-half
virtual addresses of kernel sections.

## Successful boot markers

Important serial markers expected from the current kernel are:

~~~
KBM_BOOT_OK
KBM_IDT_LOADED
KBM_LAPIC_MMIO_MAPPED
KBM_PIC_REMAP_OK
KBM_PIT_TICKS: 0x0000000000000064
KBM_MEMMAP_ENTRIES: ...
~~~

The KBM_PIT_TICKS line proves that the timer interrupt chain has run at least
100 times. Memory-map lines are printed after that stage.

## Checkpoint

1. Which boot image file does make all produce?
2. Which memory does -m 2G set?
3. Why is serial output particularly valuable in kernel development?
