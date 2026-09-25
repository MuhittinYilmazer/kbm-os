#include <stddef.h>

#include "arch/x86_64/cpu.h"
#include "arch/x86_64/paging.h"

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_HUGE (1ULL << 7)
#define ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define PAGE_OFFSET_MASK 0xFFFULL

bool paging_translate_4k(uint64_t hhdm_offset, uint64_t virtual_address,
                         uint64_t *physical_address) {
    if (physical_address == NULL) {
        return false;
    }

    uint64_t pml4_index = (virtual_address >> 39) & 0x1FFULL;
    uint64_t pdpt_index = (virtual_address >> 30) & 0x1FFULL;
    uint64_t pd_index = (virtual_address >> 21) & 0x1FFULL;
    uint64_t pt_index = (virtual_address >> 12) & 0x1FFULL;

    uint64_t cr3 = cpu_read_cr3();
    volatile uint64_t *pml4 = (volatile uint64_t *)(uintptr_t)(hhdm_offset + (cr3 & ADDRESS_MASK));

    uint64_t pml4_entry = pml4[pml4_index];
    if ((pml4_entry & PAGE_PRESENT) == 0) {
        return false;
    }

    volatile uint64_t *pdpt =
        (volatile uint64_t *)(uintptr_t)(hhdm_offset + (pml4_entry & ADDRESS_MASK));
    uint64_t pdpt_entry = pdpt[pdpt_index];
    if ((pdpt_entry & PAGE_PRESENT) == 0 || (pdpt_entry & PAGE_HUGE) != 0) {
        return false;
    }

    volatile uint64_t *pd =
        (volatile uint64_t *)(uintptr_t)(hhdm_offset + (pdpt_entry & ADDRESS_MASK));
    uint64_t pd_entry = pd[pd_index];
    if ((pd_entry & PAGE_PRESENT) == 0 || (pd_entry & PAGE_HUGE) != 0) {
        return false;
    }

    volatile uint64_t *pt =
        (volatile uint64_t *)(uintptr_t)(hhdm_offset + (pd_entry & ADDRESS_MASK));
    uint64_t pt_entry = pt[pt_index];
    if ((pt_entry & PAGE_PRESENT) == 0) {
        return false;
    }

    *physical_address = (pt_entry & ADDRESS_MASK) | (virtual_address & PAGE_OFFSET_MASK);
    return true;
}

bool paging_is_mapped(uint64_t hhdm_offset, uint64_t virtual_address) {
    uint64_t pml4_index = (virtual_address >> 39) & 0x1FFULL;
    uint64_t pdpt_index = (virtual_address >> 30) & 0x1FFULL;
    uint64_t pd_index = (virtual_address >> 21) & 0x1FFULL;
    uint64_t pt_index = (virtual_address >> 12) & 0x1FFULL;

    uint64_t cr3 = cpu_read_cr3();
    volatile uint64_t *pml4 = (volatile uint64_t *)(uintptr_t)(hhdm_offset + (cr3 & ADDRESS_MASK));
    uint64_t pml4_entry = pml4[pml4_index];
    if ((pml4_entry & PAGE_PRESENT) == 0) {
        return false;
    }

    volatile uint64_t *pdpt =
        (volatile uint64_t *)(uintptr_t)(hhdm_offset + (pml4_entry & ADDRESS_MASK));
    uint64_t pdpt_entry = pdpt[pdpt_index];
    if ((pdpt_entry & PAGE_PRESENT) == 0) {
        return false;
    }
    if ((pdpt_entry & PAGE_HUGE) != 0) {
        return true;
    }

    volatile uint64_t *pd =
        (volatile uint64_t *)(uintptr_t)(hhdm_offset + (pdpt_entry & ADDRESS_MASK));
    uint64_t pd_entry = pd[pd_index];
    if ((pd_entry & PAGE_PRESENT) == 0) {
        return false;
    }
    if ((pd_entry & PAGE_HUGE) != 0) {
        return true;
    }

    volatile uint64_t *pt =
        (volatile uint64_t *)(uintptr_t)(hhdm_offset + (pd_entry & ADDRESS_MASK));
    return (pt[pt_index] & PAGE_PRESENT) != 0;
}
