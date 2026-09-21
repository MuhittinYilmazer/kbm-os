# 13 — PMM, heap, console, keyboard, and shell

KBM now allocates physical frames, makes small heap allocations, writes a
framebuffer console, and runs a command shell from PS/2 keyboard input.

## Runtime chain

~~~text
Limine memory map → PMM → kmalloc heap → framebuffer console
                                      ↑             ↓
                             PS/2 IRQ1 keyboard → shell
~~~

PMM owns physical RAM. The heap requests PMM frames. The console writes pixel
memory. The keyboard produces interrupts. The shell turns characters into
commands.

## PMM and two bitmaps

Relevant files: `kernel/src/kernel/pmm.c`, `pmm.h`, and `main.c`.

KBM allocates 4 KiB physical frames. It keeps two bits per frame:

~~~text
primary bitmap:   0 = free / PMM may return it, 1 = used or reserved
ownership bitmap: 0 = PMM did not hand it out, 1 = PMM handed it to a caller
~~~

The ownership bitmap catches invalid free calls. A reserved frame or a frame
already freed has an unset ownership bit. `pmm_free_frame` then panics with
`KBM_FRAME_NOT_ALLOCATED`.

PMM finds `LIMINE_MEMMAP_USABLE` ranges, sizes both bitmaps from the highest
usable address, reserves bitmap storage with a temporary bump allocator, frees
only complete usable frames, and reserves bitmap metadata.

## Heap

Relevant files: `kernel/src/kernel/heap.c` and `heap.h`.

`kmalloc(size)` rounds requests to 16 bytes. If its current 4 KiB heap frame is
full, it requests another PMM frame and adds HHDM to form a C pointer. It is a
bump allocator: it has no `kfree`, and one request larger than 4 KiB panics.
That is an intentional v0.1 limit.

The `heap_get_used_bytes` and `heap_get_frame_count` functions expose heap
state to boot tests and to `MEM`.

## Verification

`main.c` verifies that:

- PMM allocation/free restores the free-frame count.
- A freed frame can be allocated again.
- The heap gets a second PMM frame when its first frame fills.
- `kmalloc(13)` returns a 16-byte-aligned pointer and adds 16 used bytes.

~~~text
KBM_PMM_OWNER_TEST_OK
KBM_PMM_FREE_REUSE_OK: ...
KBM_HEAP_SECOND_FRAME_TEST_OK: ...
KBM_HEAP_ACCOUNTING_TEST_OK
~~~

## Console, keyboard, and shell

Relevant files: `drivers/framebuffer.*`, `drivers/console.*`, `kernel/font.*`,
`drivers/keyboard.*`, and `kernel/shell.c`.

The console draws 8×8 bitmap glyphs, wraps lines, and clears instead of
scrolling. UTF-8 support is limited to ASCII and KBM's two-byte Turkish subset.
A PS/2 key press raises IRQ1, which reaches IDT vector 33 after PIC remapping.
The keyboard driver reads port `0x60`, maps a small Turkish-Q subset, sends it
to the shell, and acknowledges the PIC. v0.1 tracks only Shift state.

The shell stores up to 63 codepoints. Commands are `HELP`, `CLEAR`, `MEM`,
`TICKS`, `ECHO text`, and a small `ZEYNEP` easter egg. Values are hexadecimal
for now because that is useful for addresses and bit fields during bring-up.
