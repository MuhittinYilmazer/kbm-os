#include <stddef.h>
#include <stdint.h>

#include <limine.h>

#include "arch/x86_64/cpu.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"
#include "arch/x86_64/lapic.h"
#include "arch/x86_64/paging.h"
#include "arch/x86_64/pic.h"
#include "drivers/console.h"
#include "drivers/framebuffer.h"
#include "drivers/keyboard.h"
#include "drivers/pit.h"
#include "drivers/serial.h"
#include "kernel/font.h"
#include "kernel/heap.h"
#include "kernel/panic.h"
#include "kernel/pmm.h"
#include "kernel/shell.h"
#include "kernel/timer.h"

// Limine reads these requests before transferring control to kmain.
__attribute__((used, section(".limine_requests"))) static volatile uint64_t limine_base_revision[] =
    LIMINE_BASE_REVISION(6);

// Request a framebuffer so KBM can draw pixels without a text-mode driver.
__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
        .revision = 0,
};

// Request Limine's higher-half direct map (HHDM) for physical-memory access.
__attribute__((
    used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
};

// Request the firmware/bootloader memory map before creating the PMM bitmap.
__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
};

__attribute__((
    used,
    section(".limine_requests_start"))) static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used, section(".limine_requests_end"))) static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

static struct limine_framebuffer *boot_get_framebuffer(void) {
    // A missing framebuffer is fatal until KBM has an alternative display path.
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        panic("framebuffer not found");
    }

    return framebuffer_request.response->framebuffers[0];
}

void kmain(void) {
    // Start serial output first so every later boot failure is visible.
    serial_init();

    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) {
        panic("unsupported Limine base revision");
    }

    if (hhdm_request.response == NULL) {
        panic("HHDM not found");
    }

    if (memmap_request.response == NULL) {
        panic("memory map not found");
    }

    // Keep simple, recognizable boot markers for serial debugging.
    serial_write("KBM_BOOT_OK\n");
    serial_write("KBM_HHDM_READY\n");

    serial_write("KBM_CR3: ");
    serial_write_hex(cpu_read_cr3());
    serial_write("\n");
    serial_write("KBM_HEX_OK: ");
    serial_write_hex(9234);
    serial_write("\n");
    serial_write("\n");

    // Install KBM's CPU tables before using exceptions or hardware interrupts.
    gdt_init();
    serial_write("KBM_GDT_LOADER_CALLED\n");
    idt_init();
    serial_write("KBM_IDT_LOADED\n");
    serial_write("\n");

    // Translate one known kernel virtual address as a paging self-check.
    uint64_t hhdm_request_physical;
    if (!paging_translate_4k(hhdm_request.response->offset, (uint64_t)(uintptr_t)&hhdm_request,
                             &hhdm_request_physical)) {
        panic("paging translation failed");
    }

    serial_write("KBM_HHDM_REQUEST_PHYS: ");
    serial_write_hex(hhdm_request_physical);
    serial_write("\n");

    serial_write("KBM_PML4_511: ");
    serial_write_hex(lapic_debug_pml4_511(hhdm_request.response->offset));
    serial_write("\n");

    serial_write("KBM_PDPT_510: ");
    serial_write_hex(lapic_debug_pdpt_510(hhdm_request.response->offset));
    serial_write("\n");

    serial_write("KBM_PD_0: ");
    serial_write_hex(lapic_debug_pd_0(hhdm_request.response->offset));
    serial_write("\n");

    serial_write("KBM_PT_0: ");
    serial_write_hex(lapic_debug_pt_0(hhdm_request.response->offset));
    serial_write("\n");
    serial_write("\n");

    if (!lapic_enable_legacy_pic(hhdm_request.response->offset)) {
        panic("LAPIC MMIO mapping failed");
    }
    serial_write("KBM_LAPIC_MMIO_MAPPED\n");

    // Reserve vectors 32-47 for hardware IRQs and keep all of them masked for now.
    pic_remap();
    serial_write("KBM_PIC_REMAP_OK\n");
    pit_init();
    pic_enable_irq(0);
    asm volatile("sti");

    while (timer_get_ticks() < 100) {
        asm volatile("hlt");
    }
    asm volatile("cli");

    serial_write("KBM_PIT_TICKS: ");
    serial_write_hex(timer_get_ticks());
    serial_write("\n");
    serial_write("\n");

    serial_write("KBM_MEMMAP_ENTRIES: ");
    serial_write_hex(memmap_request.response->entry_count);
    serial_write("\n");

    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        serial_write("BASE: ");
        serial_write_hex(memmap_request.response->entries[i]->base);
        serial_write("\n");

        serial_write("LENGTH: ");
        serial_write_hex(memmap_request.response->entries[i]->length);
        serial_write("\n");

        serial_write("TYPE: ");
        serial_write_hex(memmap_request.response->entries[i]->type);
        serial_write("\n");
    }
    serial_write("\n");

    // The PMM learns usable physical frames before heap allocations begin.
    pmm_init(memmap_request, hhdm_request.response->offset);
    // Allocation followed by free must restore the PMM free-frame count.
    uint64_t free_before;
    uint64_t free_after_allocate;
    uint64_t free_after_free;

    free_before = pmm_get_free_frame_count();
    uint64_t test_frame = pmm_allocate_frame();
    free_after_allocate = pmm_get_free_frame_count();
    pmm_free_frame(test_frame);
    free_after_free = pmm_get_free_frame_count();

    if (free_after_allocate != free_before - 1) {
        panic("KBM_PMM_ALLOCATE_COUNT_ERROR");
    }

    if (free_after_free != free_before) {
        panic("KBM_PMM_FREE_COUNT_ERROR");
    }
    serial_write("KBM_PMM_OWNER_TEST_OK\n");
    serial_write("\n");

    serial_write("KBM_PMM_TOTAL_FRAMES: ");
    serial_write_hex(pmm_get_frame_count());
    serial_write("\n");

    serial_write("KBM_PMM_BITMAP_BYTES: ");
    serial_write_hex(pmm_get_bitmap_byte_count());
    serial_write("\n");
    serial_write("KBM_PMM_BITMAP_PAGES: ");
    serial_write_hex(pmm_get_bitmap_page_count());
    serial_write("\n");

    serial_write("KBM_PMM_FRAME: ");
    serial_write_hex(pmm_allocate_frame());
    serial_write("\n");
    serial_write("KBM_PMM_FRAME: ");
    serial_write_hex(pmm_allocate_frame());
    serial_write("\n");
    serial_write("KBM_PMM_FRAME: ");
    serial_write_hex(pmm_allocate_frame());
    serial_write("\n");
    serial_write("KBM_PMM_FRAME: ");
    serial_write_hex(pmm_allocate_frame());
    serial_write("\n");
    serial_write("KBM_PMM_FRAME: ");
    serial_write_hex(pmm_allocate_frame());
    serial_write("\n");

    uint64_t test_frame_physical = pmm_allocate_frame();
    volatile uint64_t *allocated_frame =
        (volatile uint64_t *)(uintptr_t)(test_frame_physical + hhdm_request.response->offset);
    *allocated_frame = 31;
    serial_write("KBM_TEST_FRAME_VALUE: ");
    serial_write_hex(*allocated_frame);
    serial_write("\n");

    pmm_free_frame(test_frame_physical);
    uint64_t recycled_frame = pmm_allocate_frame();
    if (recycled_frame != test_frame_physical) {
        panic("KBM_PMM_FREE_TEST_FAILED");
    }
    serial_write("KBM_PMM_FREE_REUSE_OK: ");
    serial_write_hex(recycled_frame);
    serial_write("\n");

    serial_write("KBM_PMM_BITMAP_PHYSICAL: ");
    serial_write_hex(pmm_get_bitmap_physical_address());
    serial_write("\n");
    serial_write("\n");

    // The first heap is a bump allocator backed by PMM-provided physical frames.
    heap_init(hhdm_request.response->offset);
    serial_write("KBM_HEAP_READY\n");

    uint64_t *first_num = kmalloc(sizeof(uint64_t));
    uint64_t *second_num = kmalloc(sizeof(uint64_t));
    uint64_t *third_num = kmalloc(sizeof(uint64_t));

    *first_num = 31;
    *second_num = 52;
    *third_num = 69;

    serial_write("KBM_HEAP_VALUE_1: ");
    serial_write_hex(*first_num);
    serial_write("\n");

    serial_write("KBM_HEAP_VALUE_2: ");
    serial_write_hex(*second_num);
    serial_write("\n");

    serial_write("KBM_HEAP_VALUE_3: ");
    serial_write_hex(*third_num);
    serial_write("\n");

    serial_write("KBM_HEAP_ADDRESS_1: ");
    serial_write_hex((uint64_t)(uintptr_t)first_num);
    serial_write("\n");

    serial_write("KBM_HEAP_ADDRESS_2: ");
    serial_write_hex((uint64_t)(uintptr_t)second_num);
    serial_write("\n");

    serial_write("KBM_HEAP_ADDRESS_3: ");
    serial_write_hex((uint64_t)(uintptr_t)third_num);
    serial_write("\n");

    // This request cannot fit in the remaining first heap frame, so it needs a new frame.
    uint64_t free_frames_before_second_heap_frame = pmm_get_free_frame_count();
    uint8_t *second_heap_frame = kmalloc(0xFF0);
    uint64_t free_frames_after_second_heap_frame = pmm_get_free_frame_count();
    second_heap_frame[0] = 0xB2;

    if (free_frames_after_second_heap_frame != free_frames_before_second_heap_frame - 1) {
        panic("KBM_HEAP_NEW_FRAME_TEST_FAILED");
    }

    if (second_heap_frame[0] != 0xB2) {
        panic("KBM_HEAP_SECOND_FRAME_TEST_FAILED");
    }

    serial_write("KBM_HEAP_SECOND_FRAME_TEST_OK: ");
    serial_write_hex(second_heap_frame[0]);
    serial_write("\n");

    // A 13-byte request is rounded to 16 bytes and starts on a 16-byte boundary.
    uint64_t heap_used_before_alignment_test = heap_get_used_bytes();
    uint8_t *alignment_test = kmalloc(13);

    if (((uint64_t)(uintptr_t)alignment_test & 0xF) != 0) {
        panic("KBM_HEAP_ALIGNMENT_TEST_FAILED");
    }

    if (heap_get_used_bytes() != heap_used_before_alignment_test + 16) {
        panic("KBM_HEAP_USAGE_TEST_FAILED");
    }

    serial_write("KBM_HEAP_ACCOUNTING_TEST_OK\n");
    serial_write("\n");

    // A software interrupt tests IDT[32] and the returning timer ISR before real IRQs exist.
    // asm volatile ("int $32");
    // The successful self-test must have incremented the timer tick count once.
    // serial_write("KBM_TIMER_SELFTEST_TICKS: ");
    // serial_write_hex(timer_get_ticks());

    // Deliberately raise #UD to test the generic fatal-exception path via IDT[6].
    // asm volatile ("ud2");

    // With memory and interrupts ready, switch from serial-only feedback to the UI.
    struct limine_framebuffer *framebuffer = boot_get_framebuffer();
    console_init(framebuffer);
    shell_init();

    keyboard_init();
    serial_write("KBM_KEYBOARD_READY\n");
    serial_write("\n");
    asm volatile("sti");

    while (true) {
        asm volatile("hlt");
    }
}
