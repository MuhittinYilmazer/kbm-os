# Freestanding C, output, and fatal failures

A normal Linux C program finds printf, malloc, libc startup code, and file or
terminal services provided by the operating system. A kernel does not run in
that environment. KBM therefore supplies its own basic needs.

## What freestanding means

In a freestanding C environment, the compiler knows the C language but does
not assume an operating system or standard C library exists. During
optimization, a compiler may still emit calls to memcpy, memset, memmove, or
memcmp. kernel/src/memory.c supplies KBM implementations of those functions.

The purpose of this file is not simply writing everything by hand. It makes the
basic memory operations emitted by the compiler resolvable.

## The COM1 serial port

At an early kernel stage, the display cannot be trusted. A framebuffer response
may be missing, the display driver may be wrong, or a page fault may happen
before screen output works. KBM therefore calls serial_init first.

drivers/serial.c uses COM1, the traditional first UART:

~~~
COM1 I/O-port base: 0x3F8
~~~

outb and inb do not use normal RAM pointers. They write or read a byte in the
x86 I/O-port address space. serial_putc waits for the UART transmit register to
become empty; serial_write sends text one character at a time; serial_write_hex
prints addresses and bit masks in fixed-width hexadecimal.

## Framebuffer drawing

When Limine provides a framebuffer response, drivers/framebuffer.c writes to
pixel memory. A framebuffer is a memory region observed by display hardware.

~~~
screen_put_pixel  -> one pixel
screen_fill       -> complete display
screen_draw_rect  -> rectangle
screen_draw_glyph -> 8x8 bitmap character
~~~

pitch is the number of bytes in a row. It need not equal width * 4 because
hardware may leave padding at the end of a row. The pixel pointer is volatile
so that writes to display memory are not removed by the compiler.

These functions are currently commented out in main.c. The framebuffer
foundation is compiled, but the normal current boot path does not draw.

## Panic and stopping

When the kernel sees a condition from which it cannot safely continue, it calls
panic. For example, continuing without a Limine HHDM or memory-map response
would make memory access assumptions invalid.

~~~
panic(message)
  -> writes KBM PANIC: ... to COM1
  -> calls cpu_halt_forever
~~~

cpu_halt_forever first uses cli to disable maskable interrupts, then enters an
infinite hlt loop. This prevents a broken kernel from continuing to run
arbitrary code.

## Checkpoint

1. Why can a freestanding kernel not rely on host libc?
2. What differs between outb and writing through a normal pointer?
3. Why can a framebuffer pointer be volatile?
4. Why does a kernel not perform a normal C return after panic?
