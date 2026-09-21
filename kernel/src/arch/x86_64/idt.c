#include <stdint.h>

// A 64-bit IDT gate stores a handler address split across three fields.
struct __attribute__((__packed__)) idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t zero;
};

// lidt reads this ten-byte descriptor in 64-bit mode.
struct __attribute__((__packed__)) idt_descriptor {
    uint16_t limit;
    uint64_t base;
};

// These assembly entry points translate CPU exceptions and IRQs into C calls.
extern void isr_divide_error();
extern void isr_invalid_opcode();
extern void isr_general_protection_fault();
extern void isr_page_fault();
extern void isr_timer();
extern void isr_keyboard();
// Implemented in idt_load.S because lidt is a CPU-specific instruction.
extern void idt_load();

void idt_set_gate(struct idt_entry *idt, uint8_t vector, void (*handler)(void)) {
    // Configure one IDT vector to transfer control to the supplied ISR.
    // The CPU-defined IDT format stores the 64-bit handler address in three pieces.
    uint64_t handler_address = (uint64_t)(uintptr_t)handler;
    idt[vector].offset_low = handler_address & 0xFFFF;
    idt[vector].offset_middle = (handler_address >> 16) & 0xFFFF;
    idt[vector].offset_high = handler_address >> 32;
    idt[vector].selector = 0x08; // KBM's Ring 0 code selector from the GDT.
    idt[vector].ist = 0;
    idt[vector].type_attributes = 0x8E; // Present, Ring 0, 64-bit interrupt gate.
    idt[vector].zero = 0;
}

void idt_init() {
    // The CPU keeps using this table after idt_init returns, so it must be static.
    // Static storage also initializes all unused IDT gates to zero.
    static struct idt_entry idt[256];

    // IDTR contains the address and byte limit of the complete IDT.
    static struct idt_descriptor idt_pointer;

    idt_pointer.base = (uint64_t)(uintptr_t)idt;
    idt_pointer.limit = sizeof(idt) - 1;

    // Install the small initial set of fatal CPU exception handlers.
    idt_set_gate(idt, 0, isr_divide_error);
    idt_set_gate(idt, 6, isr_invalid_opcode);
    idt_set_gate(idt, 13, isr_general_protection_fault);
    idt_set_gate(idt, 14, isr_page_fault);
    // PIC remapping makes legacy timer IRQ0 arrive through CPU vector 32.
    idt_set_gate(idt, 32, isr_timer);
    idt_set_gate(idt, 33, isr_keyboard);

    // Load only after every currently supported exception gate is initialized.
    idt_load(&idt_pointer);
}
