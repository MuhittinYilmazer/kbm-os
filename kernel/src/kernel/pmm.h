#ifndef KBM_KERNEL_PMM_H
#define KBM_KERNEL_PMM_H

#include <limine.h>
#include <stdint.h>

// Prepare bitmap storage and the temporary bootstrap allocator.
void pmm_init(struct limine_memmap_request memmap_request, uint64_t hhdm_offset);

// Return one 4 KiB physical frame from the temporary bump allocator.
uint64_t pmm_allocate_frame();
void pmm_free_frame(uint64_t physical_address);

// Expose bitmap information for boot-time serial debugging.
uint64_t pmm_get_frame_count();
uint64_t pmm_get_bitmap_byte_count();
uint64_t pmm_get_bitmap_page_count();
uint64_t pmm_get_bitmap_physical_address();
uint64_t pmm_get_free_frame_count();

#endif
