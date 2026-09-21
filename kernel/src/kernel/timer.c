#include "kernel/timer.h"
#include "arch/x86_64/pic.h"

// This value changes asynchronously in an interrupt handler, so it is volatile.
static volatile uint64_t ticks = 0;

void timer_irq() {
    // IRQ0 is the legacy programmable timer's hardware interrupt line.
    ticks += 1;
    // The PIC will not deliver a later IRQ until this one is acknowledged.
    pic_send_eoi(0);
}

uint64_t timer_get_ticks() {
    // At this single-core stage, a direct volatile read is sufficient.
    return ticks;
}
