# CPU state, GDT, and the stack

While KBM's C code runs, the CPU carries more than just instructions: it has
registers, a stack, segment selectors, privilege rules, and a page-table root.
The GDT is a historical but still necessary part of that state.

## New terms

- **Register:** A very small, fast storage location inside the CPU.
- **RIP:** The virtual address of the next instruction to execute.
- **RSP:** The stack pointer, which points to the top of the stack.
- **Segment selector:** A small value that selects an entry in the GDT.
- **GDT:** Global Descriptor Table; a CPU table defining segment rules.
- **Ring 0:** The kernel privilege level.
- **Long mode:** The 64-bit operating mode of x86-64.

## Registers and the stack

C function calls look simple, but use registers and the stack. In the x86-64
System V ABI, the first function argument is passed in RDI. Therefore the GDT
descriptor pointer passed to gdt_load is in RDI on the assembly side.

The stack is a LIFO structure for temporary values and return addresses:

~~~
push value  -> RSP decreases by 8 bytes, value is written to stack
pop value   -> value is read from stack, RSP increases by 8 bytes
call target -> puts a return address on stack, then jumps to target
ret         -> takes a return address from stack
~~~

A 64-bit push or pop moves 8 bytes. The CPU also puts return information on the
stack when an interrupt arrives; that is covered in the next chapter.

## What a segment is

Older x86 processors used segments to divide the address space. In 64-bit long
mode, normal code and data accesses largely do not use segment base addresses;
paging performs the primary memory-protection and translation work. The CPU
still expects valid selectors in registers such as CS, DS, ES, and SS. The
long-mode and privilege attributes of the code segment also remain important.

KBM's minimal GDT contains three entries:

~~~
GDT[0] -> null entry, required to be all zero
GDT[1] -> Ring 0 64-bit code segment, selector 0x08
GDT[2] -> Ring 0 data/stack segment, selector 0x10
~~~

A selector is not a byte address. It relates to an entry number in the GDT.
Because entries are 8 bytes, the first usable entry uses selector 0x08 and the
second uses selector 0x10.

## What gdt.c does

gdt.c defines the CPU's 8-byte gdt_entry layout and 10-byte gdt_descriptor
layout. The static array in gdt_init matters: the CPU continues to use the GDT
after the function returns, so the table cannot be temporary stack storage.

The code entry receives access value 0x9A and the data entry 0x92. These values
carry bit fields such as present, Ring 0, code or data, and writable. The code
entry's flags_limit value of 0x20 marks it as a 64-bit code segment.

## What gdt_load.S does

C cannot directly reload the CS register, so assembly is required:

~~~
cli
lgdt [rdi]
push 0x08
push the address of gdt_loaded
lretq
gdt_loaded:
  load DS, ES, and SS with 0x10
  ret
~~~

lgdt loads the GDT descriptor into the CPU, but leaves the current CS selector
unchanged. lretq is a far return: it uses the new instruction address and code
selector on the stack to change CS too. Data and stack selectors are then
reloaded.

Maskable interrupts stay disabled with cli while loading the GDT. An interrupt
must not arrive while CPU tables are only partly updated.

## Limits

KBM creates only Ring 0 kernel segments. It has no Ring 3 user mode, TSS, IST
stacks, or user/kernel transition yet. In long mode paging replaces segment
bases for most addressing, so this minimal GDT is adequate for bootstrap work,
but it is not enough for a complete user-mode system.

## Checkpoint

1. Why is RSP important as the stack pointer?
2. Why are 0x08 and 0x10 GDT selectors?
3. Why does lgdt alone not update CS?
4. Why does the GDT not disappear entirely when paging exists in long mode?
