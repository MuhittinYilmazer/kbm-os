#include "drivers/framebuffer.h"

// Write one pixel only when its coordinates are inside the visible framebuffer.
void screen_put_pixel(struct limine_framebuffer *framebuffer, uint64_t x_coordinate,
                      uint64_t y_coordinate, uint32_t color) {
    if (x_coordinate >= framebuffer->width || y_coordinate >= framebuffer->height) {
        return;
    }

    // Display hardware observes this memory, so writes must not be optimized away.
    volatile uint32_t *pixels = (volatile uint32_t *)framebuffer->address;
    // pitch may include padding, so it is not necessarily width * 4.
    uint64_t pixels_per_row = framebuffer->pitch / sizeof(*pixels);

    pixels[y_coordinate * pixels_per_row + x_coordinate] = color;
}

void screen_fill(struct limine_framebuffer *framebuffer, uint32_t color) {
    // Reuse the bounds-checked pixel primitive for every visible position.
    for (uint64_t y_coordinate = 0; y_coordinate < framebuffer->height; y_coordinate++) {
        for (uint64_t x_coordinate = 0; x_coordinate < framebuffer->width; x_coordinate++) {
            screen_put_pixel(framebuffer, x_coordinate, y_coordinate, color);
        }
    }
}

void screen_draw_rect(struct limine_framebuffer *framebuffer, uint64_t x_coordinate,
                      uint64_t y_coordinate, uint64_t width, uint64_t height, uint32_t color) {
    // Clipping is delegated to screen_put_pixel for rectangles near an edge.
    for (uint64_t y_offset = 0; y_offset < height; y_offset++) {
        for (uint64_t x_offset = 0; x_offset < width; x_offset++) {
            screen_put_pixel(framebuffer, x_coordinate + x_offset, y_coordinate + y_offset, color);
        }
    }
}

void screen_draw_glyph(struct limine_framebuffer *framebuffer, uint64_t x_coordinate,
                       uint64_t y_coordinate, const uint8_t glyph[8], uint64_t scale,
                       uint32_t color) {
    // Each byte describes one row of an 8x8 bitmap, from its most significant bit.
    for (uint64_t glyph_y = 0; glyph_y < 8; glyph_y++) {
        uint8_t row = glyph[glyph_y];

        for (uint64_t glyph_x = 0; glyph_x < 8; glyph_x++) {
            uint8_t mask = (uint8_t)(1u << (7 - glyph_x));

            // A set bit becomes a scale-by-scale colored rectangle on screen.
            if ((row & mask) != 0) {
                screen_draw_rect(framebuffer, x_coordinate + glyph_x * scale,
                                 y_coordinate + glyph_y * scale, scale, scale, color);
            }
        }
    }
}

// Move the framebuffer image up and clear the new rows at the bottom.
void screen_scroll_up(struct limine_framebuffer *framebuffer,
                      uint64_t pixel_rows,
                      uint32_t background_color) {
    if (pixel_rows >= framebuffer->height) {
        screen_fill(framebuffer, background_color);
        return;
    }

    volatile uint32_t *pixels = (volatile uint32_t *)framebuffer->address;
    uint64_t pixels_per_row = framebuffer->pitch / sizeof(*pixels);

    for (uint64_t y = 0; y < framebuffer->height - pixel_rows; y++) {
        for (uint64_t x = 0; x < pixels_per_row; x++) {
            pixels[y * pixels_per_row + x] = pixels[(y + pixel_rows) * pixels_per_row + x];
        }
    }

    for (uint64_t y = framebuffer->height - pixel_rows; y < framebuffer->height; y++) {
        for (uint64_t x = 0; x < pixels_per_row; x++) {
            pixels[y * pixels_per_row + x] = background_color;
        }
    }
}

// Scroll only the framebuffer area below top_y and leave a UI header untouched.
void screen_scroll_region_up(struct limine_framebuffer *framebuffer,
                             uint64_t top_y,
                             uint64_t pixel_rows,
                             uint32_t background_color) {
    if (top_y >= framebuffer->height) {
        return;
    }

    uint64_t region_height = framebuffer->height - top_y;
    if (pixel_rows >= region_height) {
        screen_draw_rect(framebuffer, 0, top_y, framebuffer->width, region_height,
                         background_color);
        return;
    }

    volatile uint32_t *pixels = (volatile uint32_t *)framebuffer->address;
    uint64_t pixels_per_row = framebuffer->pitch / sizeof(*pixels);

    for (uint64_t y = top_y; y < framebuffer->height - pixel_rows; y++) {
        for (uint64_t x = 0; x < pixels_per_row; x++) {
            pixels[y * pixels_per_row + x] = pixels[(y + pixel_rows) * pixels_per_row + x];
        }
    }

    for (uint64_t y = framebuffer->height - pixel_rows; y < framebuffer->height; y++) {
        for (uint64_t x = 0; x < pixels_per_row; x++) {
            pixels[y * pixels_per_row + x] = background_color;
        }
    }
}

uint32_t screen_get_pixel(struct limine_framebuffer *framebuffer, uint64_t x_coordinate,
                          uint64_t y_coordinate) {
    if (x_coordinate >= framebuffer->width || y_coordinate >= framebuffer->height) {
        return 0;
    }

    // Display hardware observes this memory, so writes must not be optimized away.
    volatile uint32_t *pixels = (volatile uint32_t *)framebuffer->address;
    // pitch may include padding, so it is not necessarily width * 4.
    uint64_t pixels_per_row = framebuffer->pitch / sizeof(*pixels);

    return pixels[y_coordinate * pixels_per_row + x_coordinate];
}
