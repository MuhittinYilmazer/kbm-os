#include <stdint.h>

#include "drivers/serial.h"

// COM1 is the conventional first PC serial port used for early kernel logs.
#define COM1 0x3F8

// These instructions access x86 I/O ports, not normal memory addresses.
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void serial_init(void) {
    // Configure COM1 for 38,400 baud with 8 data bits, no parity, and one stop bit.
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xc7);
    outb(COM1 + 4, 0x0b);
}

static void serial_putc(char character) {
    // Wait until the UART reports that its transmit register is empty.
    while ((inb(COM1 + 5) & 0x20) == 0) {
    }

    outb(COM1, (uint8_t)character);
}

void serial_write(const char *text) {
    while (*text != '\0') {
        // Terminals expect CRLF even though kernel strings use Unix-style newlines.
        if (*text == '\n') {
            serial_putc('\r');
        }

        serial_putc(*text);
        text++;
    }
}

void serial_write_hex(uint64_t value) {
    // Print every nibble so kernel addresses always have a fixed-width representation.
    static const char digits[] = "0123456789abcdef";

    serial_write("0x");

    for (int shift = 60; shift >= 0; shift -= 4) {
        uint8_t nibble = (value >> shift) & 0x0f;
        serial_putc(digits[nibble]);
    }
}
