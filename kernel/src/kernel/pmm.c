#include "kernel/pmm.h"
#include "kernel/panic.h"
#include "memory.h"

// Before the bitmap is ready, take frames from this big usable range.
static uint64_t pmm_base;
static uint64_t pmm_end;
static uint64_t next_frame_address;

// One bit means one 4 KiB physical frame.
static uint64_t bitmap_byte_count;
static uint64_t frame_count;
static uint64_t bitmap_page_count;

// We need both the physical address and the HHDM pointer of the bitmap.
static uint8_t *bitmap_pointer;
static uint64_t bitmap_physical_address;

static uint8_t bitmap_ready = 0;

// Main bitmap: can we use this frame? Ownership bitmap: did PMM give it away?
static uint8_t *ownership_bitmap_pointer;
static uint64_t ownership_bitmap_physical_address;

static void pmm_bitmap_mark_frame_free(uint64_t physical_address);
static void pmm_bitmap_mark_frame_used(uint64_t physical_address);

// Find usable RAM, build the two bitmaps, and reserve bitmap memory.
void pmm_init(struct limine_memmap_request memmap_request, uint64_t hhdm_offset) {
    uint64_t highest_usable_end = 0;
    uint64_t largest_usable_base = 0;
    uint64_t largest_usable_end = 0;
    uint64_t largest_usable_length = 0;

    // Find the highest usable address and the biggest usable range.
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        if (memmap_request.response->entries[i]->type == LIMINE_MEMMAP_USABLE) {
            // Round the start up and the end down so only complete 4 KiB frames remain.
            uint64_t usable_base = (memmap_request.response->entries[i]->base + 0xFFF) & ~0xFFF;
            uint64_t usable_end = (memmap_request.response->entries[i]->base +
                                   memmap_request.response->entries[i]->length) &
                                  ~0xFFF;
            uint64_t usable_length = usable_end - usable_base;

            if (usable_end > highest_usable_end) {
                highest_usable_end = usable_end;
            }

            if (usable_length > largest_usable_length) {
                largest_usable_base = usable_base;
                largest_usable_end = usable_end;
                largest_usable_length = usable_length;
            }
        }
    }

    // We need one bit for every frame below the highest usable address.
    frame_count = (highest_usable_end + 0xFFF) / 0x1000;
    bitmap_byte_count = (frame_count + 7) / 8;
    bitmap_page_count = (bitmap_byte_count + 0xFFF) / 0x1000;

    // The biggest range must have room for both bitmaps.
    if (largest_usable_length < bitmap_page_count * 0x1000 * 2) {
        panic("PMM bitmap does not fit in usable memory");
    }

    // Use the biggest range until the bitmap starts working.
    pmm_base = largest_usable_base;
    pmm_end = largest_usable_end;
    next_frame_address = pmm_base;

    // Take bitmap frames first, before normal allocations start.
    bitmap_physical_address = pmm_allocate_frame();

    for (uint64_t i = 0; i < bitmap_page_count - 1; i++) {
        pmm_allocate_frame();
    }

    ownership_bitmap_physical_address = pmm_allocate_frame();

    for (uint64_t i = 0; i < bitmap_page_count - 1; i++) {
        pmm_allocate_frame();
    }
    // Add HHDM so C can write to the bitmap memory.
    bitmap_pointer = (uint8_t *)(uintptr_t)(bitmap_physical_address + hhdm_offset);
    ownership_bitmap_pointer =
        (uint8_t *)(uintptr_t)(ownership_bitmap_physical_address + hhdm_offset);

    // Start with every frame blocked and owned by nobody.
    memset(bitmap_pointer, 0xFF, bitmap_byte_count);
    memset(ownership_bitmap_pointer, 0x00, bitmap_byte_count);

    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        if (memmap_request.response->entries[i]->type == LIMINE_MEMMAP_USABLE) {
            // Round the start up and the end down so only complete 4 KiB frames remain.
            uint64_t usable_base = (memmap_request.response->entries[i]->base + 0xFFF) & ~0xFFF;
            uint64_t usable_end = (memmap_request.response->entries[i]->base +
                                   memmap_request.response->entries[i]->length) &
                                  ~0xFFF;

            for (uint64_t physical_address = usable_base; physical_address < usable_end;
                 physical_address += 0x1000) {
                pmm_bitmap_mark_frame_free(physical_address);
            }
        }
    }
    // Do not give bitmap memory to normal callers.
    for (uint64_t i = 0; i < bitmap_page_count; i++) {
        uint64_t bitmap_frame_address = bitmap_physical_address + i * 0x1000;
        pmm_bitmap_mark_frame_used(bitmap_frame_address);
    }

    for (uint64_t j = 0; j < bitmap_page_count; j++) {
        uint64_t ownership_bitmap_frame_address = ownership_bitmap_physical_address + j * 0x1000;
        pmm_bitmap_mark_frame_used(ownership_bitmap_frame_address);
    }

    bitmap_ready = 1;
}

static void pmm_ownership_mark_frame_free(uint64_t physical_address) {
    // Clear bit: PMM does not currently give this frame to anyone.
    uint64_t frame_index = physical_address / 0x1000;
    uint64_t byte_index = frame_index / 8;
    uint64_t bit_index = frame_index % 8;

    ownership_bitmap_pointer[byte_index] &= ~(1 << bit_index);
}

static void pmm_ownership_mark_frame_used(uint64_t physical_address) {
    // Set bit: PMM gave this frame to someone.
    uint64_t frame_index = physical_address / 0x1000;
    uint64_t byte_index = frame_index / 8;
    uint64_t bit_index = frame_index % 8;

    ownership_bitmap_pointer[byte_index] |= (1 << bit_index);
}

static uint8_t pmm_ownership_is_frame_free(uint64_t physical_address) {
    uint64_t frame_index = physical_address / 0x1000;
    uint64_t byte_index = frame_index / 8;
    uint64_t bit_index = frame_index % 8;
    if ((ownership_bitmap_pointer[byte_index] & (1 << bit_index)) == 0) {
        return 1;
    } else {
        // Mark it used in both bitmaps, then give its address back.
        return 0;
    }
}

static void pmm_bitmap_mark_frame_free(uint64_t physical_address) {
    uint64_t frame_index = physical_address / 0x1000;
    uint64_t byte_index = frame_index / 8;
    uint64_t bit_index = frame_index % 8;

    bitmap_pointer[byte_index] &= ~(1 << bit_index);
}

static void pmm_bitmap_mark_frame_used(uint64_t physical_address) {
    uint64_t frame_index = physical_address / 0x1000;
    uint64_t byte_index = frame_index / 8;
    uint64_t bit_index = frame_index % 8;

    bitmap_pointer[byte_index] |= (1 << bit_index);
}

static uint8_t pmm_bitmap_is_frame_free(uint64_t physical_address) {
    uint64_t frame_index = physical_address / 0x1000;
    uint64_t byte_index = frame_index / 8;
    uint64_t bit_index = frame_index % 8;
    if ((bitmap_pointer[byte_index] & (1 << bit_index)) == 0) {
        return 1;
    } else {
        return 0;
    }
}

// Temporary bootstrap allocator: return the next 4 KiB frame from the chosen range.
uint64_t pmm_allocate_frame() {

    if (bitmap_ready == 0) {
        if (next_frame_address + 0x1000 <= pmm_end) {
            uint64_t frame_address = next_frame_address;
            next_frame_address += 0x1000;
            return frame_address;
        } else {
            panic("KBM_OUT_OF_MEMORY");
        }
    } else {
        uint64_t frame_index = 0;
        while (frame_index < frame_count) {
            uint64_t physical_address = frame_index * 0x1000;

            if (pmm_bitmap_is_frame_free(physical_address) == 1) {
                pmm_ownership_mark_frame_used(physical_address);
                pmm_bitmap_mark_frame_used(physical_address);
                return physical_address;
            }
            frame_index++;
        }
        panic("KBM_OUT_OF_MEMORY");
    }
}

void pmm_free_frame(uint64_t physical_address) {
    if (bitmap_ready == 0) {
        panic("KBM_BITMAP_NOT_READY");
    }
    if ((physical_address % 0x1000) != 0) {
        panic("KBM_ADDRESS_NOT_ALIGNED");
    }
    if ((physical_address / 0x1000) >= frame_count) {
        panic("KBM_OUT_OF_FRAME_COUNT");
    }
    if ((physical_address >= bitmap_physical_address) &&
        (physical_address < bitmap_physical_address + bitmap_page_count * 0x1000)) {
        panic("KBM_BETWEEN_BITMAP_ADDRESS");
    }
    // Do not free a reserved frame or a frame that was already freed.
    if (pmm_ownership_is_frame_free(physical_address) == 1) {
        panic("KBM_FRAME_NOT_ALLOCATED");
    }

    pmm_ownership_mark_frame_free(physical_address);
    pmm_bitmap_mark_frame_free(physical_address);
}

// Debug helpers used by main.c to show the bitmap layout in the serial log.
uint64_t pmm_get_frame_count() { return frame_count; }

uint64_t pmm_get_bitmap_byte_count() { return bitmap_byte_count; }

uint64_t pmm_get_bitmap_page_count() { return bitmap_page_count; }

uint64_t pmm_get_bitmap_physical_address() { return bitmap_physical_address; }

uint64_t pmm_get_free_frame_count() {
    uint64_t count = 0;
    for (uint64_t i = 0; i < frame_count; i++) {
        uint64_t physical_address = i * 0x1000;
        if (pmm_bitmap_is_frame_free(physical_address) == 1) {
            count++;
        }
    }
    return count;
}
