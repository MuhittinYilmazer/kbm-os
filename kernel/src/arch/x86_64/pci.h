#ifndef KBM_ARCH_X86_64_PCI_H
#define KBM_ARCH_X86_64_PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDRESS_PORT 0xCF8
#define PCI_CONFIG_DATA_PORT 0xCFC

// Read one aligned 32-bit value from a PCI function's configuration space.
uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t function,
                               uint8_t offset);

#endif
