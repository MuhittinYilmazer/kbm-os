#include "arch/x86_64/lapic.h"
#include "arch/x86_64/cpu.h"
#include "arch/x86_64/paging.h"

#define LAPIC_PHYSICAL_BASE 0xFEE00000ULL
#define LAPIC_VIRTUAL_BASE 0xFFFFFFFFC0000000ULL

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_WRITE_THROUGH (1ULL << 3)
#define PAGE_CACHE_DISABLE (1ULL << 4)
#define ADDRESS_MASK 0x000FFFFFFFFFF000ULL

static uint64_t lapic_page_directory[512] __attribute__((aligned(4096)));
static uint64_t lapic_page_table[512] __attribute__((aligned(4096)));

static volatile uint64_t *get_pml4(uint64_t hhdm_offset) {
    uint64_t cr3 = cpu_read_cr3();
    uint64_t pml4_physical = cr3 & ~0XFFFULL;
    volatile uint64_t *pml4_virtual = (volatile uint64_t *)(uintptr_t)(hhdm_offset + pml4_physical);
    return pml4_virtual;
}

uint64_t lapic_debug_pml4_511(uint64_t hhdm_offset) {
    volatile uint64_t *pml4 = get_pml4(hhdm_offset);
    return pml4[511];
}

uint64_t lapic_debug_pdpt_510(uint64_t hhdm_offset) {
    uint64_t pml4_511 = lapic_debug_pml4_511(hhdm_offset);
    uint64_t pdpt_physical = pml4_511 & 0x000FFFFFFFFFF000UL;
    volatile uint64_t *pdpt_virtual = (volatile uint64_t *)(uintptr_t)(pdpt_physical + hhdm_offset);
    return pdpt_virtual[510];
}

uint64_t lapic_debug_pd_0(uint64_t hhdm_offset) {
    uint64_t pdpt_510 = lapic_debug_pdpt_510(hhdm_offset);
    uint64_t pd_physical = pdpt_510 & 0x000FFFFFFFFFF000UL;
    volatile uint64_t *pd_virtual = (volatile uint64_t *)(uintptr_t)(pd_physical + hhdm_offset);
    return pd_virtual[0];
}

uint64_t lapic_debug_pt_0(uint64_t hhdm_offset) {
    uint64_t pd_0 = lapic_debug_pd_0(hhdm_offset);
    uint64_t pt_physical = pd_0 & 0x000FFFFFFFFFF000ULL;
    volatile uint64_t *pt_virtual = (volatile uint64_t *)(uintptr_t)(pt_physical + hhdm_offset);
    return pt_virtual[0];
}

bool lapic_enable_legacy_pic(uint64_t hhdm_offset) {
    uint64_t page_directory_physical;
    uint64_t page_table_physical;

    if (!paging_translate_4k(hhdm_offset, (uint64_t)(uintptr_t)lapic_page_directory,
                             &page_directory_physical)) {
        return false;
    }

    if (!paging_translate_4k(hhdm_offset, (uint64_t)(uintptr_t)lapic_page_table,
                             &page_table_physical)) {
        return false;
    }

    volatile uint64_t *pml4 = get_pml4(hhdm_offset);
    uint64_t pml4_entry = pml4[511];
    if ((pml4_entry & PAGE_PRESENT) == 0) {
        return false;
    }

    uint64_t pdpt_physical = pml4_entry & ADDRESS_MASK;
    volatile uint64_t *pdpt_virtual = (volatile uint64_t *)(uintptr_t)(pdpt_physical + hhdm_offset);

    if ((pdpt_virtual[511] & PAGE_PRESENT) != 0) {
        return false;
    }

    for (uint64_t i = 0; i < 512; i++) {
        lapic_page_directory[i] = 0;
        lapic_page_table[i] = 0;
    }

    pdpt_virtual[511] = page_directory_physical | PAGE_PRESENT | PAGE_WRITABLE;

    lapic_page_directory[0] = page_table_physical | PAGE_PRESENT | PAGE_WRITABLE;

    lapic_page_table[0] = LAPIC_PHYSICAL_BASE | PAGE_PRESENT | PAGE_WRITABLE | PAGE_WRITE_THROUGH |
                          PAGE_CACHE_DISABLE;

    asm volatile("invlpg (%0)" : : "r"(LAPIC_VIRTUAL_BASE) : "memory");

    volatile uint32_t *lapic = (volatile uint32_t *)(uintptr_t)LAPIC_VIRTUAL_BASE;
    uint32_t lint0 = lapic[0x350 / 4];

    lint0 &= ~(0x7U << 8);
    lint0 |= (0x7U << 8);
    lint0 &= ~(1U << 16);

    lapic[0x350 / 4] = lint0;
    return true;
}
