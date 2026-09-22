#include "kernel/heap.h"
#include "kernel/panic.h"
#include "kernel/pmm.h"

#define HEAP_BLOCK_MAGIC 0x4B424D48


struct heap_block {
    uint64_t size;
    struct heap_block *next;
    uint64_t is_free;
    uint64_t magic;
};

static uint64_t heap_hhdm_offset;
static uint8_t heap_ready;
static uint64_t heap_used_bytes;
static uint64_t heap_frame_count;
// This points to the first available block that kmalloc can reuse.
static struct heap_block *heap_free_list;

static uint64_t round_up_to_16(uint64_t size) {
    return ((size + 0xF) & ~0xF);
}

void heap_init(uint64_t hhdm_offset) {
    heap_hhdm_offset = hhdm_offset;
    heap_ready = 1;
    heap_used_bytes = 0;
    heap_frame_count = 0;
    heap_free_list = 0;
}

void *kmalloc(uint64_t size) {
    if (heap_ready == 0) {
        panic("KBM_HEAP_NOT_INITIATED");
    }

    if (size == 0) {
        panic("KBM_0_HEAP_ALLOCATION");
    }

    size = round_up_to_16(size);

    if (size > ((0x1000) - sizeof(struct heap_block))) {
        panic("KBM_HEAP_OVERFLOW");
    }

    struct heap_block *previous = 0;
    struct heap_block *block = heap_free_list;

    while (block != 0) {
        if (block->is_free == 1 && block->size >= size) {
            break;
        } else {
            previous = block;
            block = block->next;
        }
    }

    if (block == 0) {
        struct heap_block *new_frame_header =
            (struct heap_block *)(uintptr_t)(pmm_allocate_frame() + heap_hhdm_offset);
        new_frame_header->size = 0x1000 - sizeof(struct heap_block);
        new_frame_header->is_free = 1;
        new_frame_header->magic = HEAP_BLOCK_MAGIC;

        // Keep the free-list ordered by address so adjacent blocks can be merged later.
        previous = 0;
        block = heap_free_list;

        while (block != 0 &&
               (uint64_t)(uintptr_t)block < (uint64_t)(uintptr_t)new_frame_header) {
            previous = block;
            block = block->next;
        }

        new_frame_header->next = block;
        if (previous == 0) {
            heap_free_list = new_frame_header;
        } else {
            previous->next = new_frame_header;
        }

        block = new_frame_header;
        heap_frame_count++;
    }

    uint64_t remaining = block->size - size;

    if (remaining >= sizeof(struct heap_block) + 16) {
        struct heap_block *new_block =
            (struct heap_block *)((uint8_t *)(block + 1) + size);
        new_block->size = remaining - sizeof(struct heap_block);
        new_block->next = block->next;
        new_block->is_free = 1;
        new_block->magic = HEAP_BLOCK_MAGIC;

        if (previous == 0) {
            heap_free_list = new_block;
        } else {
            previous->next = new_block;
        }

        block->size = size;
    } else {
        if (previous == 0) {
            heap_free_list = block->next;
        } else {
            previous->next = block->next;
        }
    }

    block->is_free = 0;
    block->next = 0;

    heap_used_bytes += block->size;

    return (void *)(block + 1);
}

void kfree(void *pointer) {
    if (pointer == 0) {
        return;
    }

    if (heap_ready == 0) {
        panic("KBM_HEAP_NOT_INITIATED");
    }

    struct heap_block *block = (struct heap_block *)pointer - 1;

    if (block->magic != HEAP_BLOCK_MAGIC) {
        panic("KBM_HEAP_INVALID_FREE");
    }

    if (block->is_free != 0) {
        panic("KBM_HEAP_DOUBLE_FREE");
    }

    // Remember the allocated payload size before merging changes this block's size.
    uint64_t freed_size = block->size;
    block->is_free = 1;

    // Insert the freed block in address order instead of always putting it first.
    struct heap_block *previous = 0;
    struct heap_block *current = heap_free_list;

    while (current != 0 &&
           (uint64_t)(uintptr_t)current < (uint64_t)(uintptr_t)block) {
        previous = current;
        current = current->next;
    }

    block->next = current;
    if (previous == 0) {
        heap_free_list = block;
    } else {
        previous->next = block;
    }

    // If the following free block starts right after this payload, combine them.
    if (block->next != 0 &&
        (uint8_t *)(block + 1) + block->size == (uint8_t *)block->next) {
        block->size += sizeof(struct heap_block) + block->next->size;
        block->next = block->next->next;
    }

    // The previous block may also end exactly where this header begins.
    if (previous != 0 &&
        (uint8_t *)(previous + 1) + previous->size == (uint8_t *)block) {
        previous->size += sizeof(struct heap_block) + block->size;
        previous->next = block->next;
    }

    heap_used_bytes -= freed_size;
}

// MEM and the boot tests read these counters.
uint64_t heap_get_used_bytes() { return heap_used_bytes; }
uint64_t heap_get_frame_count() { return heap_frame_count; }
