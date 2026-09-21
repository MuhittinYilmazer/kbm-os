#ifndef KBM_KERNEL_PANIC_H
#define KBM_KERNEL_PANIC_H

// Panic never returns because execution cannot safely continue afterward.
void panic(const char *message) __attribute__((noreturn));

#endif
