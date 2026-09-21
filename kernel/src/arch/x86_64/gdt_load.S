.intel_syntax noprefix

.global gdt_load

gdt_load:
    # The kernel owns interrupt setup, so keep maskable interrupts disabled.
    cli

    # RDI is the first C argument: a pointer to the GDT descriptor.
    lgdt [rdi]

    # CS cannot be reloaded with mov. Build a far-return frame with the new
    # code selector and a RIP-relative continuation address instead.
    mov rax, 0x08
    push rax
    lea rax, [rip + gdt_loaded]
    push rax
    lretq

gdt_loaded:
    # Reload the data and stack selectors from KBM's own GDT.
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    ret
