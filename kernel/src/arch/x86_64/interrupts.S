.intel_syntax noprefix

.global isr_divide_error
.global isr_invalid_opcode
.global isr_general_protection_fault
.global isr_page_fault
.global isr_timer
.global isr_keyboard

.extern exception_fatal
.extern timer_irq
.extern keyboard_irq

isr_divide_error:
    # Pass the CPU-assigned exception vector as the first C argument.
    mov rdi, 0
    jmp isr_errors

isr_invalid_opcode:
    mov rdi, 6
    jmp isr_errors

isr_general_protection_fault:
    mov rdi, 13
    jmp isr_errors

isr_page_fault:
    mov rdi, 14
    jmp isr_errors

isr_timer:
    # C may overwrite caller-saved registers, so preserve interrupted state.
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11

    # Keep the original stack address before realigning for the SysV C ABI.
    push rbx
    cld
    mov rbx, rsp
    and rsp, -16

    # The C layer increments ticks and acknowledges IRQ0 at the PIC.
    call timer_irq

    # Discard only alignment padding and restore the saved register frame.
    mov rsp, rbx
    pop rbx
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax

    # A hardware IRQ returns to the instruction it interrupted.
    iretq

isr_keyboard:
    # Keyboard IRQ1 follows the same C-call and register-preservation rules.
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push rbx
    cld
    mov rbx, rsp
    and rsp, -16

    call keyboard_irq

    mov rsp, rbx
    pop rbx
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    iretq

isr_errors:
    # C code expects the direction flag clear and a 16-byte-aligned stack.
    cld
    and rsp, -16

    # Current exception handlers are fatal, so their CPU frames are not restored.
    call exception_fatal

halt:
    cli
    hlt
    jmp halt
