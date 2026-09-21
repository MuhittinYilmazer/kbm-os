.intel_syntax noprefix

.global idt_load

idt_load:
    # RDI points to the IDT descriptor prepared by idt_init.
    lidt [rdi]
    ret
