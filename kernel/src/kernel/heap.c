#include "kernel/heap.h"
#include "kernel/panic.h"
#include "kernel/pmm.h"

static uint64_t heap_hhdm_offset;
// This points to the next free byte in the current heap frame.
static uint8_t *heap_current;
static uint8_t *heap_end;
static uint8_t heap_ready;
static uint64_t heap_used_bytes;
static uint64_t heap_frame_count;

void heap_init(uint64_t hhdm_offset) {
    heap_hhdm_offset = hhdm_offset;
    heap_current = 0;
    heap_end = 0;
    heap_ready = 1;
    heap_used_bytes = 0;
    heap_frame_count = 0;
}

void *kmalloc(uint64_t size) {
    if (heap_ready == 0) {
        panic("KBM_HEAP_NOT_INITIATED");
    }

    if (size == 0) {
        panic("KBM_0_HEAP_ALLOCATION");
    } else if (size > 0x1000) {
        panic("KBM_HEAP_OVERFLOW");
    }

    // Round up to 16 bytes so returned pointers stay aligned.
    size = (size + 0xF) & ~0xF;

    // If this frame is full, get a new one from PMM.
    if ((heap_current == 0) || (heap_current + size > heap_end)) {
        heap_current = (uint8_t *)(uintptr_t)(pmm_allocate_frame() + heap_hhdm_offset);
        heap_frame_count++;
        heap_end = heap_current + 0x1000;
    }

    void *result = heap_current;
    heap_current = heap_current + size;
    heap_used_bytes += size;

    return result;
}

// MEM and the boot tests read these counters.
uint64_t heap_get_used_bytes() { return heap_used_bytes; }
uint64_t heap_get_frame_count() { return heap_frame_count; }
