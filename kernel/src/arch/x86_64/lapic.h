#ifndef KBM_ARCH_X86_64_LAPIC
#define KBM_ARCH_X86_64_LAPIC

#include <stdbool.h>
#include <stdint.h>

bool lapic_enable_legacy_pic(uint64_t hhdm_offset);
uint64_t lapic_debug_pml4_511(uint64_t hhdm_offset);
uint64_t lapic_debug_pdpt_510(uint64_t hhdm_offset);
uint64_t lapic_debug_pd_0(uint64_t hhdm_offset);
uint64_t lapic_debug_pt_0(uint64_t hhdm_offset);

#endif
