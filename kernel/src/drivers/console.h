#ifndef KBM_DRIVERS_CONSOLE_H
#define KBM_DRIVERS_CONSOLE_H

#include <limine.h>
#include <stdint.h>

void console_init(struct limine_framebuffer *framebuffer);
void console_write_char(char character);
void console_write_codepoint(uint32_t character);
void console_write(const char *text);
void console_write_hex(uint64_t value);
void console_backspace(void);
void console_clear(void);

#endif
