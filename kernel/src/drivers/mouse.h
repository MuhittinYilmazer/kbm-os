#ifndef KBM_DRIVERS_MOUSE_H
#define KBM_DRIVERS_MOUSE_H

#include <stdint.h>
#include <limine.h>

void mouse_init(struct limine_framebuffer *framebuffer);
void mouse_irq();

#endif
