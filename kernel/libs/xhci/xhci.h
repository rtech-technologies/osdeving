#ifndef XHCI_H
#define XHCI_H

#include "types.h"

// xHCI Capability Registers
typedef struct {
    uint8  cap_length;
    uint8  reserved;
    uint16 hci_version;
    uint32 hcs_params1;
    uint32 hcs_params2;
    uint32 hcs_params3;
    uint32 hcc_params1;
    uint32 dba_offset;
    uint32 rts_offset;
    uint32 hcc_params2;
} xhci_cap_regs_t;

// xHCI Operational Registers
typedef struct {
    uint32 usb_cmd;
    uint32 usb_sts;
    uint32 page_size;
    uint32 reserved1[2];
    uint32 dn_ctrl;
    uint64 crcr;
    uint32 reserved2[4];
    uint64 dcbaap;
    uint32 config;
} xhci_op_regs_t;

// xHCI Port Registers
typedef struct {
    uint32 portsc;
    uint32 portpmsc;
    uint32 portli;
    uint32 reserved;
} xhci_port_regs_t;

// xHCI TRB (Transfer Request Block)
typedef struct {
    uint64 parameter;
    uint32 status;
    uint32 control;
} xhci_trb_t;

void xhci_init();
void xhci_poll();
void xhci_enable_slot();

#endif
