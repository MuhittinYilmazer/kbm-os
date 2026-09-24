#ifndef KBM_DRIVERS_CONSOLE_H
#define KBM_DRIVERS_CONSOLE_H

#include <limine.h>
#include <stdint.h>

void console_init(struct limine_framebuffer *framebuffer);
void console_write_char(char character);
void console_write_codepoint(uint32_t character);
void console_write(const char *text);
void console_write_hex(uint64_t value);
void console_write_decimal(uint64_t value);
void console_backspace(void);
void console_clear(void);
void console_set_text_color(uint32_t color);
void console_set_background_color(uint32_t color);
void console_update_header(uint64_t uptime_seconds);

#endif
