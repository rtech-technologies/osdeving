#include "xhci.h"
#include "console.h"
#include "memory.h"
#include "usb_keyboard.h"
#include "input_map.h"
#include "../../unice64/io.h"

#define PCI_CLASS_SERIAL 0x0C
#define PCI_SUBCLASS_USB  0x03
#define PCI_PROGIF_XHCI   0x30

static uint64 xhci_base = 0;
static xhci_cap_regs_t* caps = NULL;
static xhci_op_regs_t* ops = NULL;
static xhci_port_regs_t* ports = NULL;

static xhci_trb_t* cmd_ring = NULL;
static uint32 cmd_ring_index = 0;
static uint8 cmd_cycle = 1;

static xhci_trb_t* event_ring = NULL;
static uint32 event_ring_index = 0;
static uint8 event_cycle = 1;

typedef struct {
    uint64 rsba;
    uint32 rsz;
    uint32 reserved;
} erst_entry_t;

static erst_entry_t* erst = NULL;
static uint64* dcbaap = NULL;

// Device Transfer Rings (v0: simplified single slot/ep)
static xhci_trb_t* xfer_ring = NULL;
static uint32 xfer_index = 0;
static uint8 xfer_cycle = 1;
static uint8* hid_buffer = NULL;

void xhci_init() {
    if (xhci_base) return;

    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            uint32 v = pci_read_config_32(bus, dev, 0, 0);
            if ((v & 0xFFFF) == 0xFFFF) continue;
            uint32 c = pci_read_config_32(bus, dev, 0, 0x08);
            if (((c >> 24) & 0xFF) == PCI_CLASS_SERIAL && ((c >> 16) & 0xFF) == PCI_SUBCLASS_USB && ((c >> 8) & 0xFF) == PCI_PROGIF_XHCI) {
                uint32 bar0 = pci_read_config_32(bus, dev, 0, 0x10);
                uint32 bar1 = pci_read_config_32(bus, dev, 0, 0x14);
                xhci_base = ((uint64)bar1 << 32) | (bar0 & 0xFFFFFFF0);
                break;
            }
        }
        if (xhci_base) break;
    }

    if (!xhci_base) return;

    caps = (xhci_cap_regs_t*)xhci_base;
    ops = (xhci_op_regs_t*)(xhci_base + caps->cap_length);
    ports = (xhci_port_regs_t*)((uint8*)ops + 0x400);

    ops->usb_cmd &= ~1;
    while (!(ops->usb_sts & (1 << 0)));

    ops->usb_cmd |= (1 << 1);
    while (ops->usb_cmd & (1 << 1));
    while (ops->usb_sts & (1 << 11));

    uint32 hcs1 = caps->hcs_params1;
    uint32 max_slots = hcs1 & 0xFF;
    ops->config = max_slots;

    dcbaap = (uint64*)memory_alloc(2048);
    for(int i=0; i<256; i++) dcbaap[i] = 0;
    ops->dcbaap = (uint64)dcbaap;

    cmd_ring = (xhci_trb_t*)memory_alloc(4096);
    for(int i=0; i<256; i++) cmd_ring[i].control = 0;
    ops->crcr = (uint64)cmd_ring | cmd_cycle;

    erst = (erst_entry_t*)memory_alloc(sizeof(erst_entry_t));
    event_ring = (xhci_trb_t*)memory_alloc(4096);
    for(int i=0; i<256; i++) event_ring[i].control = 0;
    erst->rsba = (uint64)event_ring;
    erst->rsz = 256;

    uint64 runtime_base = xhci_base + caps->rts_offset;
    mmio_write32(runtime_base + 0x28, 1);
    mmio_write64(runtime_base + 0x30, (uint64)erst);
    mmio_write64(runtime_base + 0x38, (uint64)event_ring | (1 << 3));

    ops->usb_cmd |= 1;
    while (ops->usb_sts & (1 << 0));

    hid_buffer = (uint8*)memory_alloc(64);
    xfer_ring = (xhci_trb_t*)memory_alloc(4096);
    for(int i=0; i<256; i++) xfer_ring[i].control = 0;

    console_print("Native xHCI Driver Operational.\n");
}

static void xhci_doorbell(uint32 slot, uint32 target) {
    uint32* doorbells = (uint32*)(xhci_base + caps->dba_offset);
    doorbells[slot] = target;
}

void xhci_submit_cmd(uint64 param, uint32 status, uint32 ctrl) {
    cmd_ring[cmd_ring_index].parameter = param;
    cmd_ring[cmd_ring_index].status = status;
    cmd_ring[cmd_ring_index].control = (ctrl & ~1) | cmd_cycle;

    cmd_ring_index++;
    if (cmd_ring_index == 255) {
        cmd_ring[255].control = (6 << 10) | cmd_cycle | (1 << 5);
        cmd_ring_index = 0;
        cmd_cycle = !cmd_cycle;
    }
    xhci_doorbell(0, 0);
}

void xhci_submit_xfer(uint64 param, uint32 len, uint32 ctrl) {
    xfer_ring[xfer_index].parameter = param;
    xfer_ring[xfer_index].status = len;
    xfer_ring[xfer_index].control = (ctrl & ~1) | xfer_cycle;

    xfer_index++;
    if (xfer_index == 255) {
        xfer_ring[255].control = (6 << 10) | xfer_cycle | (1 << 5);
        xfer_index = 0;
        xfer_cycle = !xfer_cycle;
    }
    xhci_doorbell(1, 1); // Slot 1, Endpoint 1 (Mocked)
}

void xhci_process_events() {
    if (!event_ring) return;
    uint64 runtime_base = xhci_base + caps->rts_offset;

    while ((event_ring[event_ring_index].control & 1) == event_cycle) {
        xhci_trb_t* trb = &event_ring[event_ring_index];
        uint32 type = (trb->control >> 10) & 0x3F;

        if (type == 32) { // Transfer Event
            if ((trb->status >> 24) == 1) { // Success
                 usb_keyboard_process_report(hid_buffer, 8);
                 // Resubmit for next report
                 xhci_submit_xfer((uint64)hid_buffer, 8, (1 << 10) | (1 << 6)); // Normal TRB, IOC
            }
        } else if (type == 33) { // Command Completion
            uint32 slot = trb->control >> 24;
            if (slot) {
                dcbaap[slot] = (uint64)xfer_ring; // Simplified assignment
                input_map_set_status(INPUT_SRC_USB_HID, INPUT_STATUS_CONNECTED);
                // Trigger first transfer
                xhci_submit_xfer((uint64)hid_buffer, 8, (1 << 10) | (1 << 6));
            }
        }

        event_ring_index++;
        if (event_ring_index == 256) {
            event_ring_index = 0;
            event_cycle = !event_cycle;
        }
        mmio_write64(runtime_base + 0x38, (uint64)&event_ring[event_ring_index] | (1 << 3));
    }
}

void xhci_poll() {
    if (!xhci_base) return;
    uint32 port_count = (caps->hcs_params1 >> 24) & 0xFF;
    for (uint32 i = 0; i < port_count; i++) {
        uint32 sc = ports[i].portsc;
        if (sc & (1 << 17)) { // CSC
            ports[i].portsc |= (1 << 17);
            if (sc & 1) {
                ports[i].portsc |= (1 << 4); // PR
            }
        }
        if (sc & (1 << 21)) { // PRC
            ports[i].portsc |= (1 << 21);
            if (sc & (1 << 1)) { // PED
                xhci_submit_cmd(0, 0, (9 << 10)); // Enable Slot
            }
        }
    }
    xhci_process_events();
}
