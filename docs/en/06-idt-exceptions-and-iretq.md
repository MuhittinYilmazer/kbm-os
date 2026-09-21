# IDT, exceptions, and iretq

The GDT defines CPU segment rules. The IDT tells the CPU which code to execute
when an exception or interrupt arrives.

## New terms

- **IDT:** Interrupt Descriptor Table; a handler table for 256 possible vectors.
- **Vector:** A number from 0 to 255 used by the CPU to select a handler.
- **Exception:** An event produced during the CPU's own execution.
- **IRQ:** An interrupt request produced by external hardware.
- **ISR:** Interrupt Service Routine; the handler entered by the CPU.
- **iretq:** The 64-bit interrupt-return instruction.

## Difference between an exception and IRQ

An exception arises from the CPU's own instruction execution:

~~~
0  -> divide error
6  -> invalid opcode
13 -> general protection fault
14 -> page fault
~~~

An IRQ comes from external hardware such as a timer, keyboard, or network card.
KBM remaps the legacy PIC so timer IRQ0 reaches the CPU as vector 32. The same
IDT serves both exceptions and IRQs, although their source and entry stack can
differ.

## IDT gate structure

An x86-64 IDT entry stores a handler's 64-bit address in three fields:
offset_low, offset_middle, and offset_high. idt_set_gate fills those pieces and
writes KBM's code-segment selector, 0x08, to the selector field.

The type_attributes value 0x8E means:

~~~
present
Ring 0
64-bit interrupt gate
~~~

idt_init creates a static table with 256 entries. It currently installs gates
for four fatal exceptions and timer vector 32. The lidt instruction in
idt_load.S loads the address and limit of this table into IDTR.

## What the CPU does when an interrupt arrives

For the timer, the path is:

~~~
CPU executes normal code
  -> an interrupt is accepted
  -> CPU puts return information on the stack
  -> CPU enters the address in IDT[32]
  -> isr_timer assembly executes
  -> timer_irq C function is called
  -> isr_timer returns through iretq
~~~

The CPU's saved stack information includes the interrupted RIP, CS, and RFLAGS.
If privilege level changes, the old RSP and SS are added. Some exceptions also
produce an error code. These details matter when writing handlers.

## interrupts.S and the ABI

Before calling C code, isr_timer saves caller-saved registers: RAX, RCX, RDX,
RSI, RDI, and R8-R11. A C call may overwrite them. RBX stores the original
stack address, and the stack is aligned to 16 bytes. The System V ABI expects
that alignment before a C call.

After timer_irq returns, registers are restored in reverse order. The final
iretq is not an ordinary ret. ret only takes a normal function return address.
iretq restores the CPU's pre-interrupt RIP, CS, and RFLAGS state from the
stack and resumes the interrupted code.

## Fatal exception path

isr_divide_error, isr_invalid_opcode, isr_general_protection_fault, and
isr_page_fault put a vector number in RDI and jump to shared isr_errors.
exception_fatal prints the vector number over serial and halts the CPU. These
handlers intentionally do not return with iretq because KBM currently treats
those exceptions as unrecoverable.

## Limits

There is no generic handler that parses every exception error-code frame, no
IST stack, no user-mode exception return, and no interrupt-nesting support yet.
The current path is designed to make early kernel failures visible and to
return safely from the timer IRQ.

## Checkpoint

1. What differs between an exception and an IRQ?
2. Why does the timer use IDT[32]?
3. Why does stack alignment matter before a C call?
4. Why can iretq not be replaced with ret?
