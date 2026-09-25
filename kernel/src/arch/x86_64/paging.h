#ifndef KBM_ARCH_X86_64_PAGING_H
#define KBM_ARCH_X86_64_PAGING_H

#include <stdbool.h>
#include <stdint.h>

bool paging_translate_4k(uint64_t hhdm_offset, uint64_t virtual_address,
                         uint64_t *physical_address);
bool paging_is_mapped(uint64_t hhdm_offset, uint64_t virtual_address);

#endif
