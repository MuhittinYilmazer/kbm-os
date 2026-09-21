#ifndef KBM_DRIVERS_SERIAL_H
#define KBM_DRIVERS_SERIAL_H

#include <stdint.h>

// Early debug-console API backed by the x86 COM1 UART.
void serial_init(void);
void serial_write(const char *text);
void serial_write_hex(uint64_t value);

#endif
