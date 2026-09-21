#ifndef KBM_KERNEL_TIMER
#define KBM_KERNEL_TIMER

#include <stdint.h>

// Advance KBM's software clock after a timer IRQ reaches the kernel.
void timer_irq(void);
// Read the number of timer interrupts handled since boot.
uint64_t timer_get_ticks();

#endif
