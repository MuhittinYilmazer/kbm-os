#include "drivers/console.h"
#include "drivers/framebuffer.h"
#include "kernel/font.h"

static struct limine_framebuffer *console_framebuffer;
static uint64_t console_cursor_x;
static uint64_t console_cursor_y;
static uint64_t console_glyph_scale = 2;
static uint32_t console_text_color = 0x00ffffff;
static uint32_t console_background_color = 0x00101018;

void console_init(struct limine_framebuffer *framebuffer) {
    // The console draws directly into the framebuffer supplied by Limine.
    console_framebuffer = framebuffer;
    console_cursor_x = 0;
    console_cursor_y = 0;

    screen_fill(console_framebuffer, console_background_color);
}

void console_write_codepoint(uint32_t character) {
    uint64_t glyph_width = 8 * console_glyph_scale;
    uint64_t glyph_height = 8 * console_glyph_scale;

    if (character == '\n') {
        console_cursor_x = 0;
        console_cursor_y += glyph_height;
        return;
    }

    const uint8_t *glyph = font_get_glyph(character);
    if (glyph == 0) {
        return;
    }

    if (console_cursor_x + glyph_width > console_framebuffer->width) {
        console_cursor_x = 0;
        console_cursor_y += glyph_height;
    }

    // No scrolling yet: clear the screen when we reach the bottom.
    if (console_cursor_y + glyph_height > console_framebuffer->height) {
        screen_fill(console_framebuffer, console_background_color);
        console_cursor_x = 0;
        console_cursor_y = 0;
    }

    screen_draw_glyph(console_framebuffer, console_cursor_x, console_cursor_y, glyph,
                      console_glyph_scale, console_text_color);

    console_cursor_x += glyph_width;
}

void console_write_char(char character) { console_write_codepoint((uint8_t)character); }

void console_write(const char *text) {
    // Read normal ASCII and the two-byte Turkish UTF-8 letters we need.
    uint64_t index = 0;

    while (text[index] != '\0') {
        uint8_t first_byte = (uint8_t)text[index];

        if (first_byte < 0x80) {
            console_write_codepoint(first_byte);
            index++;
        } else {
            uint8_t second_byte = (uint8_t)text[index + 1];

            // Turkish Latin letters use two-byte UTF-8 sequences.
            if ((first_byte & 0xe0) == 0xc0 && (second_byte & 0xc0) == 0x80) {
                uint32_t codepoint =
                    ((uint32_t)(first_byte & 0x1f) << 6) | (uint32_t)(second_byte & 0x3f);
                console_write_codepoint(codepoint);
                index += 2;
            } else {
                // Unsupported or malformed UTF-8 is visible as a question mark.
                console_write_codepoint('?');
                index++;
            }
        }
    }
}

void console_write_hex(uint64_t value) {
    // KBM's font currently contains uppercase letters, so use 0X and A-F.
    static const char digits[] = "0123456789ABCDEF";

    console_write("0X");

    for (int shift = 60; shift >= 0; shift -= 4) {
        uint8_t nibble = (value >> shift) & 0x0f;
        console_write_char(digits[nibble]);
    }
}

void console_backspace(void) {
    // Backspace only erases one character on the current line.
    uint64_t glyph_width = 8 * console_glyph_scale;
    uint64_t glyph_height = 8 * console_glyph_scale;

    if (console_cursor_x == 0) {
        return;
    }

    console_cursor_x -= glyph_width;

    screen_draw_rect(console_framebuffer, console_cursor_x, console_cursor_y, glyph_width,
                     glyph_height, console_background_color);
}

void console_clear(void) {
    screen_fill(console_framebuffer, console_background_color);
    console_cursor_x = 0;
    console_cursor_y = 0;
}
