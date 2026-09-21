#ifndef KBM_KERNEL_SHELL_H
#define KBM_KERNEL_SHELL_H

#include <stdint.h>

void shell_init();
void shell_handle_character(uint32_t character);

#endif
