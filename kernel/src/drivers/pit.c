#include "drivers/pit.h"
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Keep this legacy I/O delay private to the PIT driver.
static inline void io_wait_pit(void) { outb(0x80, 0); }

#define PIT_CLOCK_SPEED 1193182
#define PIT_FREQUENCY 100

void pit_init(void) {
    // Mode 3 generates a periodic square-wave interrupt at approximately 100 Hz.
    uint16_t divisor = PIT_CLOCK_SPEED / PIT_FREQUENCY;

    outb(0x43, 0x36);
    io_wait_pit();

    outb(0x40, divisor & 0xFF);
    io_wait_pit();
    outb(0x40, (divisor >> 8) & 0xFF);
    io_wait_pit();
}
