#ifndef KBM_ARCH_X86_64_PIC
#define KBM_ARCH_X86_64_PIC

#include <stdint.h>

// Move hardware IRQs away from CPU exception vectors before enabling them.
void pic_remap(void);
// Tell the PIC that KBM finished handling one IRQ.
void pic_send_eoi(uint8_t irq);

void pic_enable_irq(uint8_t irq);

#endif
