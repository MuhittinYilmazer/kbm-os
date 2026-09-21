#include "drivers/keyboard.h"
#include "arch/x86_64/pic.h"
#include "drivers/serial.h"
#include "kernel/shell.h"
#include "stdint.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// This first layout only needs to remember whether a Shift key is held.
static int keyboard_shift_down;

// Translate a small Set 1 subset into ASCII and Turkish keyboard characters.
static uint32_t keyboard_scancode_to_character(uint8_t scan_code) {
    switch (scan_code) {
    case 0x02:
        return keyboard_shift_down ? '!' : '1';
    case 0x03:
        return '2';
    case 0x04:
        return '3';
    case 0x05:
        return keyboard_shift_down ? '+' : '4';
    case 0x06:
        return '5';
    case 0x07:
        return '6';
    case 0x08:
        return keyboard_shift_down ? '/' : '7';
    case 0x09:
        return '8';
    case 0x0a:
        return '9';
    case 0x0b:
        return keyboard_shift_down ? '=' : '0';
    case 0x0c:
        return keyboard_shift_down ? '?' : '*';
    case 0x0d:
        return keyboard_shift_down ? '_' : '-';
    case 0x0e:
        return '\b';
    case 0x10:
        return 'Q';
    case 0x11:
        return 'W';
    case 0x12:
        return 'E';
    case 0x13:
        return 'R';
    case 0x14:
        return 'T';
    case 0x15:
        return 'Y';
    case 0x16:
        return 'U';
    case 0x17:
        return 'I';
    case 0x18:
        return 'O';
    case 0x19:
        return 'P';
    case 0x1c:
        return '\n';
    case 0x1e:
        return 'A';
    case 0x1f:
        return 'S';
    case 0x20:
        return 'D';
    case 0x21:
        return 'F';
    case 0x22:
        return 'G';
    case 0x23:
        return 'H';
    case 0x24:
        return 'J';
    case 0x25:
        return 'K';
    case 0x26:
        return 'L';
    case 0x2c:
        return 'Z';
    case 0x2d:
        return 'X';
    case 0x2e:
        return 'C';
    case 0x2f:
        return 'V';
    case 0x30:
        return 'B';
    case 0x31:
        return 'N';
    case 0x32:
        return 'M';
    // Turkish-Q special-letter and punctuation positions.
    case 0x1a:
        return keyboard_shift_down ? 0x011e : 0x011f; // ğ Ğ
    case 0x1b:
        return keyboard_shift_down ? 0x00dc : 0x00fc; // ü Ü
    case 0x27:
        return keyboard_shift_down ? 0x015e : 0x015f; // ş Ş
    case 0x28:
        return keyboard_shift_down ? 0x0130 : 'I'; // i İ
    case 0x2b:
        return keyboard_shift_down ? '/' : ',';
    case 0x33:
        return keyboard_shift_down ? 0x00d6 : 0x00f6; // ö Ö
    case 0x34:
        return keyboard_shift_down ? 0x00c7 : 0x00e7; // ç Ç
    case 0x35:
        return keyboard_shift_down ? ':' : '.';
    case 0x56:
        return keyboard_shift_down ? '>' : '<';
    case 0x39:
        return ' ';
    default:
        return 0;
    }
}

void keyboard_init() { pic_enable_irq(1); }

void keyboard_irq() {
    // PS/2 keyboard data arrives on legacy PIC IRQ1 through I/O port 0x60.
    uint8_t scan_code = inb(0x60);

    // Set 1 scan-codes for left/right Shift press and release.
    if (scan_code == 0x2a || scan_code == 0x36) {
        keyboard_shift_down = 1;
        pic_send_eoi(1);
        return;
    }

    if (scan_code == 0xaa || scan_code == 0xb6) {
        keyboard_shift_down = 0;
        pic_send_eoi(1);
        return;
    }

    uint32_t character = keyboard_scancode_to_character(scan_code);

    if (character != 0) {
        shell_handle_character(character);
    }

    serial_write("KBM_KEYBOARD_SCANCODE: ");
    serial_write_hex(scan_code);
    serial_write("\n");
    pic_send_eoi(1);
}
