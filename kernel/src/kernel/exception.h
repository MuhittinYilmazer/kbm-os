#ifndef KBM_KERNEL_EXCEPTION
#define KBM_KERNEL_EXCEPTION

#include <stdint.h>

// Fatal policy shared by KBM's currently installed CPU exception handlers.
void exception_fatal(uint64_t err_vector_num);

#endif
