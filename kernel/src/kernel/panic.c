#include "kernel/panic.h"
#include "arch/x86_64/cpu.h"
#include "drivers/serial.h"

// Report an unrecoverable kernel error before halting the CPU.
void panic(const char *message) {
    serial_write("KBM PANIC: ");
    serial_write(message);
    serial_write("\n");

    cpu_halt_forever();
}
