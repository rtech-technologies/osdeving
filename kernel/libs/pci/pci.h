#ifndef PCI_H
#define PCI_H

#include "types.h"

typedef struct {
    uint8 bus;
    uint8 slot;
    uint8 func;
    uint16 vendor_id;
    uint16 device_id;
    uint8 class_code;
    uint8 sub_class;
} pci_device_t;

void pci_init();
void pci_ls();
int pci_get_devices(pci_device_t* list, int max);

#endif
