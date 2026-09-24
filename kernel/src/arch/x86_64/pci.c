#include "arch/x86_64/pci.h"

// Write one 32-bit value to an x86 I/O port.
static inline void outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

// Read one 32-bit value from an x86 I/O port.
static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t function,
                               uint8_t offset) {
    uint32_t address = 0x80000000
                       | ((uint32_t)bus << 16)
                       | ((uint32_t)device << 11)
                       | ((uint32_t)function << 8)
                       | ((uint32_t)offset & 0xFC);

    outl(PCI_CONFIG_ADDRESS_PORT, address);
    return inl(PCI_CONFIG_DATA_PORT);
}
