#include "usb_keyboard.h"
#include "console.h"
#include "input_map.h"
#include "../../unice64/io.h"
#include "../../unice64/kernel.h"

// --- Standard USB HID Definitions ---
typedef struct {
    uint8  Length;
    uint8  DescriptorType;
    uint8  EndpointAddress;
    uint8  Attributes;
    uint16 MaxPacketSize;
    uint8  Interval;
} USB_ENDPOINT_DESCRIPTOR;

typedef struct {
    uint8  Length;
    uint8  DescriptorType;
    uint8  InterfaceNumber;
    uint8  AlternateSetting;
    uint8  NumEndpoints;
    uint8  InterfaceClass;
    uint8  InterfaceSubClass;
    uint8  InterfaceProtocol;
    uint8  Interface;
} USB_INTERFACE_DESCRIPTOR;

typedef struct _EFI_USB_IO_PROTOCOL EFI_USB_IO_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_USB_IO_GET_INTERFACE_DESCRIPTOR)(
    IN EFI_USB_IO_PROTOCOL           *This,
    OUT USB_INTERFACE_DESCRIPTOR     *InterfaceDescriptor
);

typedef EFI_STATUS (EFIAPI *EFI_USB_IO_GET_ENDPOINT_DESCRIPTOR)(
    IN EFI_USB_IO_PROTOCOL           *This,
    IN UINT8                         EndpointIndex,
    OUT USB_ENDPOINT_DESCRIPTOR      *EndpointDescriptor
);

typedef EFI_STATUS (EFIAPI *EFI_USB_IO_SYNC_INTERRUPT_TRANSFER)(
    IN EFI_USB_IO_PROTOCOL           *This,
    IN UINT8                         DeviceEndpoint,
    IN OUT void                      *Data,
    IN OUT UINTN                     *DataLength,
    IN UINTN                         Timeout,
    OUT UINT32                       *Status
);

struct _EFI_USB_IO_PROTOCOL {
    void* ControlTransfer;
    void* BulkTransfer;
    void* InterruptTransfer;
    EFI_USB_IO_SYNC_INTERRUPT_TRANSFER SyncInterruptTransfer;
    void* GetDeviceDescriptor;
    void* GetConfigDescriptor;
    EFI_USB_IO_GET_INTERFACE_DESCRIPTOR UsbGetInterfaceDescriptor;
    EFI_USB_IO_GET_ENDPOINT_DESCRIPTOR UsbGetEndpointDescriptor;
};

// --- Driver State ---
#define MAX_KEYBOARDS 4
static struct {
    EFI_USB_IO_PROTOCOL* io;
    uint8 endpoint;
    uint8 last_report[8];
} keyboards[MAX_KEYBOARDS];

static int keyboard_count = 0;

void usb_init_controller(void) {
    console_print("Probing PCI for USB Host Controllers...\n");
    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            uint32 v = pci_read_config_32(bus, dev, 0, 0);
            if ((v & 0xFFFF) == 0xFFFF) continue;
            uint32 c = pci_read_config_32(bus, dev, 0, 0x08);
            if (((c >> 24) & 0xFF) == 0x0C && ((c >> 16) & 0xFF) == 0x03) {
                uint8 pi = (c >> 8) & 0xFF;
                console_print("USB Controller Found: ");
                if (pi == 0x30) console_print("xHCI\n");
                else if (pi == 0x20) console_print("EHCI\n");
                else console_print("Legacy\n");
            }
        }
    }
}

void usb_enumerate_hid_keyboard(void) {
    boot_params_t* p = get_boot_params();
    if (!p || !p->st) return;

    EFI_GUID usb_io_g = { 0x2B2D2436, 0x9831, 0x442D, { 0x8D, 0x6D, 0xEF, 0x1A, 0x5D, 0x01, 0xD1, 0x89 } };
    UINTN count = 0;
    EFI_HANDLE* handles = NULL;

    EFI_STATUS s = uefi_call_wrapper(p->st->BootServices->LocateHandleBuffer, 5, ByProtocol, &usb_io_g, NULL, &count, &handles);
    if (s != EFI_SUCCESS) return;

    keyboard_count = 0;
    for (UINTN i = 0; i < count && keyboard_count < MAX_KEYBOARDS; i++) {
        EFI_USB_IO_PROTOCOL* usb_io = NULL;
        if (uefi_call_wrapper(p->st->BootServices->HandleProtocol, 3, handles[i], &usb_io_g, (void**)&usb_io) == EFI_SUCCESS) {
            USB_INTERFACE_DESCRIPTOR ifd;
            if (uefi_call_wrapper(usb_io->UsbGetInterfaceDescriptor, 2, usb_io, &ifd) == EFI_SUCCESS) {
                // Class 3: HID, Protocol 1: Keyboard
                if (ifd.InterfaceClass == 3 && ifd.InterfaceProtocol == 1) {
                    // Find Interrupt-In Endpoint
                    for (int e = 0; e < ifd.NumEndpoints; e++) {
                        USB_ENDPOINT_DESCRIPTOR ed;
                        if (uefi_call_wrapper(usb_io->UsbGetEndpointDescriptor, 3, usb_io, (uint8)e, &ed) == EFI_SUCCESS) {
                            if ((ed.EndpointAddress & 0x80) && (ed.Attributes & 0x03) == 0x03) {
                                keyboards[keyboard_count].io = usb_io;
                                keyboards[keyboard_count].endpoint = ed.EndpointAddress;
                                for(int k=0; k<8; k++) keyboards[keyboard_count].last_report[k] = 0;
                                keyboard_count++;
                                console_print("USB Keyboard Bound (Endpoint 0x");
                                // Simple hex print placeholder
                                char buf[4]; buf[0] = '0'; buf[1] = 'x'; buf[2] = '0' + (ed.EndpointAddress >> 4); buf[3] = 0;
                                console_print(buf);
                                console_print(")\n");
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    uefi_call_wrapper(p->st->BootServices->FreePool, 1, handles);
}

void usb_poll_all() {
    for (int i = 0; i < keyboard_count; i++) {
        uint8 report[8];
        UINTN len = 8;
        UINT32 status = 0;
        EFI_STATUS s = uefi_call_wrapper(keyboards[i].io->SyncInterruptTransfer, 6,
                                        keyboards[i].io, keyboards[i].endpoint, report, &len, 0, &status);

        if (s == EFI_SUCCESS && len == 8) {
            // Process modifiers (report[0]) and up to 6 keys (report[2..7])
            // For simplicity in a kernel loop, we handle the first new key found
            for (int k = 2; k < 8; k++) {
                if (report[k] != 0) {
                    // Check if this key was in the last report
                    int found = 0;
                    for (int prev = 2; prev < 8; prev++) {
                        if (report[k] == keyboards[i].last_report[prev]) { found = 1; break; }
                    }
                    if (!found) {
                        input_map_push(INPUT_SRC_USB_HID, report[k], report[0]);
                    }
                }
            }
            for (int k = 0; k < 8; k++) keyboards[i].last_report[k] = report[k];
        }
    }
}

int usb_has_key(void) {
    usb_poll_all();
    return input_map_has_char();
}

int usb_get_key(void) {
    usb_poll_all();
    return (int)input_map_pop_char();
}

void usb_lsdev(void) {
    console_print("--- OSx2 Device Inventory ---\n");
    usb_init_controller();
    if (keyboard_count == 0) {
        console_print("No active USB HID Keyboards found.\n");
    } else {
        console_print("Active USB HID Keyboards: ");
        // Simple int to string
        char buf[4];
        buf[0] = '0' + keyboard_count;
        buf[1] = '\n';
        buf[2] = 0;
        console_print(buf);
    }
}

void usb_keyboard_init(void) {
    usb_init_controller();
    usb_enumerate_hid_keyboard();
}
