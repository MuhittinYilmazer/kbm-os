#include "arch/x86_64/reboot.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void reboot(void) {
    asm volatile("cli");

    // Wait until the keyboard controller can accept a command.
    while ((inb(0x64) & 0x02) != 0) {
    }

    // Ask the legacy keyboard controller to reset the CPU.
    outb(0x64, 0xFE);

    // Do not return to the shell if the reset command fails.
    for (;;) {
        asm volatile("hlt");
    }
}
