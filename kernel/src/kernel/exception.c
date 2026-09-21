#include "exception.h"
#include "arch/x86_64/cpu.h"
#include "drivers/serial.h"
#include <stdint.h>

void exception_fatal(uint64_t err_vector_num) {
    // Every currently installed exception is treated as a fatal kernel error.
    serial_write("KBM_EXCEPTION: VECTOR ");
    serial_write_hex(err_vector_num);
    serial_write("\n");
    cpu_halt_forever();
}
