#include "drivers/console.h"
#include "drivers/framebuffer.h"
#include "kernel/font.h"

static struct limine_framebuffer *console_framebuffer;
static uint64_t console_cursor_x;
static uint64_t console_cursor_y;
static uint64_t console_glyph_scale = 2;
static uint32_t console_text_color = 0x00ffffff;
static uint32_t console_background_color = 0x00101018;
static uint64_t console_header_uptime_seconds;

#define CONSOLE_HEADER_HEIGHT 40
#define CONSOLE_CONTENT_TOP 56
#define CONSOLE_HEADER_COLOR 0x00161C24
#define CONSOLE_ACCENT_COLOR 0x004DD9C8
#define CONSOLE_HEADER_LINE_COLOR 0x002B3440
#define CONSOLE_DIM_COLOR 0x008B949E

// Use this colour for text written after this call.
void console_set_text_color(uint32_t color) { console_text_color = color; }

// Use this colour when clearing or scrolling console rows.
void console_set_background_color(uint32_t color) { console_background_color = color; }

// Header text can use a separate scale without moving the normal console cursor.
static void console_draw_header_text(uint64_t x_coordinate, uint64_t y_coordinate,
                                     const char *text, uint64_t scale, uint32_t color) {
    uint64_t index = 0;

    while (text[index] != '\0') {
        const uint8_t *glyph = font_get_glyph((uint8_t)text[index]);
        if (glyph != 0) {
            screen_draw_glyph(console_framebuffer, x_coordinate, y_coordinate, glyph, scale,
                              color);
        }
        x_coordinate += 8 * scale;
        index++;
    }
}

// The header keeps real uptime visible without taking space from the shell itself.
void console_update_header(uint64_t uptime_seconds) {
    char uptime_text[] = "00:00:00";
    uint64_t hours = uptime_seconds / 3600;
    uint64_t minutes = (uptime_seconds / 60) % 60;
    uint64_t seconds = uptime_seconds % 60;

    if (hours > 99) {
        hours = 99;
    }

    uptime_text[0] = '0' + (hours / 10);
    uptime_text[1] = '0' + (hours % 10);
    uptime_text[3] = '0' + (minutes / 10);
    uptime_text[4] = '0' + (minutes % 10);
    uptime_text[6] = '0' + (seconds / 10);
    uptime_text[7] = '0' + (seconds % 10);

    console_header_uptime_seconds = uptime_seconds;
    screen_draw_rect(console_framebuffer, 0, 0, console_framebuffer->width,
                     CONSOLE_HEADER_HEIGHT, CONSOLE_HEADER_COLOR);
    screen_draw_rect(console_framebuffer, 0, CONSOLE_HEADER_HEIGHT - 1,
                     console_framebuffer->width, 1, CONSOLE_HEADER_LINE_COLOR);

    // The small block is aligned to the text instead of being a full-height side bar.
    screen_draw_rect(console_framebuffer, 16, 12, 3, 16, CONSOLE_ACCENT_COLOR);
    console_draw_header_text(28, 12, "kbm", 2, console_text_color);

    uint64_t uptime_width = 8 * 16;
    uint64_t uptime_x = 16;
    if (console_framebuffer->width > uptime_width + 16) {
        uptime_x = console_framebuffer->width - uptime_width - 16;
    }
    console_draw_header_text(uptime_x, 12, uptime_text, 2, CONSOLE_DIM_COLOR);
}

void console_init(struct limine_framebuffer *framebuffer) {
    // The console draws directly into the framebuffer supplied by Limine.
    console_framebuffer = framebuffer;
    console_cursor_x = 0;
    console_cursor_y = CONSOLE_CONTENT_TOP;

    screen_fill(console_framebuffer, console_background_color);
    console_update_header(0);
}

// Start a new text line and scroll when there is no room left.
static void console_new_line(uint64_t glyph_height) {
    console_cursor_x = 0;
    console_cursor_y += glyph_height;

    if (console_cursor_y + glyph_height > console_framebuffer->height) {
        screen_scroll_region_up(console_framebuffer, CONSOLE_CONTENT_TOP, glyph_height,
                                console_background_color);
        console_cursor_y -= glyph_height;
    }
}

void console_write_codepoint(uint32_t character) {
    uint64_t glyph_width = 8 * console_glyph_scale;
    uint64_t glyph_height = 8 * console_glyph_scale;

    if (character == '\n') {
        console_new_line(glyph_height);
        return;
    }

    const uint8_t *glyph = font_get_glyph(character);
    if (glyph == 0) {
        return;
    }

    if (console_cursor_x + glyph_width > console_framebuffer->width) {
        console_new_line(glyph_height);
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

void console_write_decimal(uint64_t value) {
    if (value == 0) {
        console_write_char('0');
        return;
    }

    char digits[20];
    uint64_t digit_count = 0;

    while (value > 0) {
        uint64_t last_digit = value % 10;
        digits[digit_count] = '0' + last_digit;
        value = value / 10;
        digit_count++;
    }

    while (digit_count > 0) {
        digit_count--;
        console_write_char(digits[digit_count]);
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
    console_cursor_y = CONSOLE_CONTENT_TOP;
    console_update_header(console_header_uptime_seconds);
}
