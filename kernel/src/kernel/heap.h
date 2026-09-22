#ifndef KBM_KERNEL_HEAP_H
#define KBM_KERNEL_HEAP_H

#include <stdint.h>

void heap_init(uint64_t hhdm_offset);
void *kmalloc(uint64_t size);
void kfree(void *pointer);
uint64_t heap_get_used_bytes();
uint64_t heap_get_frame_count();

#endif
