#include "drivers/mouse.h"
#include "drivers/serial.h"
#include "drivers/framebuffer.h"
#include "arch/x86_64/pic.h"
#include <stdint.h>

#define MOUSE_CURSOR_SIZE 4
#define MOUSE_CURSOR_COLOR 0x00FF0000
#define MOUSE_CURSOR_BACKGROUND 0x00101018

static uint8_t mouse_packet[3];
static uint8_t mouse_packet_index;
static int32_t mouse_x = 0;
static int32_t mouse_y = 0;
static int32_t mouse_old_x;
static int32_t mouse_old_y;
static uint64_t mouse_screen_width;
static uint64_t mouse_screen_height;
struct limine_framebuffer *mouse_framebuffer;

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    // outb writes one byte to an x86 I/O port rather than normal memory.
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void mouse_wait_to_write() {
    // Bit 1 means the controller is still busy with an older command.
    while ((inb(0x64) & 0x02) != 0) {
    }
}

static void mouse_wait_to_read() {
    // Bit 0 means the controller has placed a byte in the data port.
    while ((inb(0x64) & 0x01) == 0) {
    }
}

static void mouse_discard_pending_data() {
    // The keyboard and mouse share port 0x60. Do not mistake an old input byte
    // for the controller command byte requested below.
    while ((inb(0x64) & 0x01) != 0) {
        inb(0x60);
    }
}

static uint8_t mouse_send_command(uint8_t command) {
    // 0xD4 sends the next data-port byte to the auxiliary PS/2 mouse.
    mouse_wait_to_write();
    outb(0x64, 0xD4);
    mouse_wait_to_write();
    outb(0x60, command);
    mouse_wait_to_read();
    return inb(0x60);
}

void mouse_init(struct limine_framebuffer *framebuffer) {
	mouse_framebuffer = framebuffer;
	mouse_screen_width = framebuffer->width;
	mouse_screen_height = framebuffer->height;
	mouse_x = mouse_screen_width / 2;
	mouse_y = mouse_screen_height / 2;
	mouse_old_x = mouse_x;
	mouse_old_y = mouse_y;

	screen_draw_rect(
    mouse_framebuffer,
    mouse_x,
    mouse_y,
    MOUSE_CURSOR_SIZE,
    MOUSE_CURSOR_SIZE,
    MOUSE_CURSOR_COLOR
  );


    // Enable the controller's auxiliary PS/2 port.
    mouse_wait_to_write();
    outb(0x64, 0xA8);

    // Keep both PS/2 devices enabled while turning on mouse IRQ12.
    mouse_discard_pending_data();
    mouse_wait_to_write();
    outb(0x64, 0x20);
    mouse_wait_to_read();
    uint8_t command_byte = inb(0x60);
    command_byte |= 0x03;
    command_byte &= (uint8_t)~0x30;

    mouse_wait_to_write();
    outb(0x64, 0x60);
    mouse_wait_to_write();
    outb(0x60, command_byte);

    uint8_t defaults_response = mouse_send_command(0xF6);
    if (defaults_response != 0xFA) {
        serial_write("KBM_MOUSE_DEFAULTS_RESPONSE: ");
        serial_write_hex(defaults_response);
        serial_write("\n");
    }

    uint8_t reporting_response = mouse_send_command(0xF4);
    if (reporting_response != 0xFA) {
        serial_write("KBM_MOUSE_REPORTING_RESPONSE: ");
        serial_write_hex(reporting_response);
        serial_write("\n");
    }

    pic_enable_irq(12);
    serial_write("KBM_MOUSE_IRQ_READY\n");
}

void mouse_irq() {
    // A standard PS/2 mouse sends one movement report as three separate bytes.
    uint8_t mouse_byte = inb(0x60);

    // QEMU can deliver a delayed command-acknowledgement after interrupts begin.
    if (mouse_packet_index == 0 && mouse_byte == 0xFA) {
        pic_send_eoi(12);
        return;
    }

    // The first byte always has bit 3 set. Ignore stray bytes until a packet starts.
    if (mouse_packet_index == 0 && (mouse_byte & 0x08) == 0) {
        pic_send_eoi(12);
        return;
    }

    mouse_packet[mouse_packet_index] = mouse_byte;
    mouse_packet_index++;

    if (mouse_packet_index == 3) {
        if ((mouse_packet[0] & 0xC0) == 0) {
            int8_t x_movement = (int8_t)mouse_packet[1];
            int8_t y_movement = (int8_t)mouse_packet[2];
            mouse_x += x_movement;
            mouse_y -= y_movement;

            if (mouse_x < 0) {
                mouse_x = 0;
            }

            if (mouse_y < 0) {
                mouse_y = 0;
            }

            if ((uint64_t)mouse_x > mouse_screen_width - MOUSE_CURSOR_SIZE) {
                mouse_x = mouse_screen_width - MOUSE_CURSOR_SIZE;
            }

            if ((uint64_t)mouse_y > mouse_screen_height - MOUSE_CURSOR_SIZE) {
                mouse_y = mouse_screen_height - MOUSE_CURSOR_SIZE;
            }

            screen_draw_rect(
    mouse_framebuffer,
    mouse_old_x,
    mouse_old_y,
    MOUSE_CURSOR_SIZE,
    MOUSE_CURSOR_SIZE,
    MOUSE_CURSOR_BACKGROUND
  );

	screen_draw_rect(
    mouse_framebuffer,
    mouse_x,
    mouse_y,
    MOUSE_CURSOR_SIZE,
    MOUSE_CURSOR_SIZE,
    MOUSE_CURSOR_COLOR
  );
            mouse_old_x = mouse_x;
            mouse_old_y = mouse_y;
        }
        serial_write("KBM_MOUSE_PACKET: ");
        serial_write_hex(mouse_packet[0]);
        serial_write(" ");
        serial_write_hex(mouse_packet[1]);
        serial_write(" ");
        serial_write_hex(mouse_packet[2]);
        serial_write("\n");
        mouse_packet_index = 0;
    }

    pic_send_eoi(12);
}
