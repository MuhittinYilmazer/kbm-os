#include "arch/x86_64/pic.h"
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    // outb writes one byte to an x86 I/O port rather than normal memory.
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Keep this legacy I/O delay private to the PIC driver.
static inline void io_wait(void) { outb(0x80, 0); }

void pic_remap(void) {
    // ICW1: initialize both legacy 8259 PICs and request ICW4.
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    // ICW2: master IRQs use vectors 32-39; slave IRQs use vectors 40-47.
    outb(0x21, 0x20);
    io_wait();
    outb(0xA1, 0x28);
    io_wait();

    // ICW3: IRQ2 connects the slave PIC to the master's cascade input.
    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();

    // ICW4: use 8086/88-compatible mode.
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    // Mask every hardware IRQ until KBM has installed and enabled its handler.
    outb(0x21, 0xFF);
    io_wait();
    outb(0xA1, 0xFF);
    io_wait();
}

void pic_send_eoi(uint8_t irq) {
    // Slave IRQs first need an EOI at the slave, then at the master cascade.
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }

    // Every IRQ, including a slave IRQ, passes through the master PIC.
    outb(0x20, 0x20);
}

void pic_enable_irq(uint8_t irq) {
    if (irq < 8) {
        uint8_t current_mask = inb(0x21);
        uint8_t irq_bit = (uint8_t)(1 << irq);
        current_mask &= (uint8_t)~irq_bit;
        outb(0x21, current_mask);
    } else if (irq >= 8 && irq < 16) {
        uint8_t current_mask = inb(0xA1);
        uint8_t irq_bit = (uint8_t)(1 << (irq - 8));
        current_mask &= (uint8_t)~irq_bit;
        outb(0xA1, current_mask);

        // The slave PIC reaches the CPU through the master's IRQ2 cascade line.
        current_mask = inb(0x21);
        current_mask &= (uint8_t)~(1 << 2);
        outb(0x21, current_mask);
    }
}
