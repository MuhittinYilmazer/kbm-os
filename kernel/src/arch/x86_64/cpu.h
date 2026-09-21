#ifndef KBM_ARCH_X86_64_CPU_H
#define KBM_ARCH_X86_64_CPU_H

#include <stdint.h>

// Stop normal execution permanently after a fatal kernel error.
__attribute__((noreturn)) static inline void cpu_halt_forever(void) {
    // Block maskable interrupts before entering the infinite hlt loop.
    asm volatile("cli");

    for (;;) {
        asm volatile("hlt");
    }
}

static inline uint64_t cpu_read_cr3(void) {
    uint64_t value;
    asm volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

#endif
