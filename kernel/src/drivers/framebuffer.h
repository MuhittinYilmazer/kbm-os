#ifndef KBM_DRIVERS_FRAMEBUFFER_H
#define KBM_DRIVERS_FRAMEBUFFER_H

#include <stdint.h>

#include <limine.h>

// Minimal framebuffer drawing primitives used by KBM's early graphics code.
void screen_put_pixel(struct limine_framebuffer *framebuffer, uint64_t x_coordinate,
                      uint64_t y_coordinate, uint32_t color);
void screen_fill(struct limine_framebuffer *framebuffer, uint32_t color);
void screen_draw_rect(struct limine_framebuffer *framebuffer, uint64_t x_coordinate,
                      uint64_t y_coordinate, uint64_t width, uint64_t height, uint32_t color);
void screen_draw_glyph(struct limine_framebuffer *framebuffer, uint64_t x_coordinate,
                       uint64_t y_coordinate, const uint8_t glyph[8], uint64_t scale,
                       uint32_t color);
void screen_scroll_up(struct limine_framebuffer *framebuffer,
                      uint64_t pixel_rows,
                      uint32_t background_color);
uint32_t screen_get_pixel(struct limine_framebuffer *framebuffer, uint64_t x_coordinate,
                          uint64_t y_coordinate);


#endif
