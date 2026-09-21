#include <stdint.h>

// A GDT entry must match the eight-byte layout expected by the CPU.
struct __attribute__((__packed__)) gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t flags_limit;
    uint8_t base_high;
};

// lgdt reads this ten-byte descriptor in 64-bit mode.
struct __attribute__((__packed__)) gdt_descriptor {
    uint16_t limit;
    uint64_t base;
};

// Implemented in gdt_load.S because C cannot reload CS directly.
void gdt_load(struct gdt_descriptor *gdt_pointer);

void gdt_init() {
    // Static storage keeps the table alive after this function returns.
    // Entry zero starts all zero, as required by the GDT convention.
    static struct gdt_entry gdt[3];

    // Entry one: the 64-bit Ring 0 code segment selected by 0x08.
    gdt[1].limit_low = 0;
    gdt[1].base_low = 0;
    gdt[1].base_middle = 0;
    gdt[1].access = 0x9A;
    gdt[1].flags_limit = 0x20;
    gdt[1].base_high = 0;

    // Entry two: the Ring 0 data and stack segment selected by 0x10.
    gdt[2].limit_low = 0;
    gdt[2].base_low = 0;
    gdt[2].base_middle = 0;
    gdt[2].access = 0x92;
    gdt[2].flags_limit = 0x00;
    gdt[2].base_high = 0;

    // The CPU needs both the table address and its size minus one.
    static struct gdt_descriptor gdt_pointer;

    gdt_pointer.base = (uint64_t)(uintptr_t)gdt;
    gdt_pointer.limit = sizeof(gdt) - 1;

    gdt_load(&gdt_pointer);
}
