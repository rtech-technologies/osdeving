#include "usb_keyboard.h"
#include "console.h"
#include "keyboard_map.h"
#include "../../unice64/io.h"
#include "../../unice64/kernel.h"

// Minimum UEFI USB definitions for a "real" driver backend
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
};

#define USB_KEY_BUFFER_SIZE 32
static uint8 key_buffer[USB_KEY_BUFFER_SIZE];
static int buffer_start = 0;
static int buffer_end = 0;

static EFI_USB_IO_PROTOCOL* active_usb_keyboard = NULL;
static uint8 last_hid_report[8] = {0};

uint8 translate_hid_to_keycode(uint8 hid_code) {
    if (hid_code >= 0x04 && hid_code <= 0x1D) return 'a' + (hid_code - 0x04);
    if (hid_code >= 0x1E && hid_code <= 0x27) {
        if (hid_code == 0x27) return '0';
        return '1' + (hid_code - 0x1E);
    }
    if (hid_code == 0x28) return '\n';
    if (hid_code == 0x2C) return ' ';
    if (hid_code == 0x2A) return '\b';
    return 0;
}

void usb_init_controller(void) {
    console_print("Scanning for USB Controllers via PCI...\n");
    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            uint32 val = pci_read_config_32(bus, dev, 0, 0);
            if ((val & 0xFFFF) == 0xFFFF) continue;
            uint32 class_rev = pci_read_config_32(bus, dev, 0, 0x08);
            if (((class_rev >> 24) & 0xFF) == 0x0C && ((class_rev >> 16) & 0xFF) == 0x03) {
                uint8 prog_if = (class_rev >> 8) & 0xFF;
                const char* type = (prog_if == 0x30) ? "XHCI" : (prog_if == 0x20) ? "EHCI" : "Legacy";
                console_print("Detected USB Controller: ");
                console_print(type);
                console_print("\n");
            }
        }
    }
}

void usb_enumerate_hid_keyboard(void) {
    boot_params_t* params = get_boot_params();
    if (!params || !params->st) return;

    EFI_GUID usb_io_g = { 0x2B2D2436, 0x9831, 0x442D, { 0x8D, 0x6D, 0xEF, 0x1A, 0x5D, 0x01, 0xD1, 0x89 } };
    UINTN count = 0;
    EFI_HANDLE* handles = NULL;

    EFI_STATUS status = uefi_call_wrapper(params->st->BootServices->LocateHandleBuffer, 5,
                                          ByProtocol, &usb_io_g, NULL, &count, &handles);

    if (status == EFI_SUCCESS && count > 0) {
        for (UINTN i = 0; i < count; i++) {
            EFI_USB_IO_PROTOCOL* usb_io = NULL;
            status = uefi_call_wrapper(params->st->BootServices->HandleProtocol, 3, handles[i], &usb_io_g, (void**)&usb_io);
            if (status == EFI_SUCCESS && usb_io) {
                USB_INTERFACE_DESCRIPTOR if_desc;
                status = uefi_call_wrapper(usb_io->UsbGetInterfaceDescriptor, 2, usb_io, &if_desc);
                if (status == EFI_SUCCESS && if_desc.InterfaceClass == 3 && if_desc.InterfaceProtocol == 1) {
                    console_print("Direct USB Keyboard link established.\n");
                    active_usb_keyboard = usb_io;
                    break;
                }
            }
        }
        uefi_call_wrapper(params->st->BootServices->FreePool, 1, handles);
    }
}

static void usb_poll_direct() {
    if (!active_usb_keyboard) return;
    uint8 report[8];
    UINTN len = 8;
    UINT32 status = 0;
    // Use 0ms timeout for non-blocking poll
    EFI_STATUS s = uefi_call_wrapper(active_usb_keyboard->SyncInterruptTransfer, 6,
                                    active_usb_keyboard, 0x81, report, &len, 0, &status);

    if (s == EFI_SUCCESS && len == 8) {
        if (report[2] != last_hid_report[2] && report[2] != 0) {
            char c = translate_hid_to_keycode(report[2]);
            if (c) {
                int next = (buffer_end + 1) % USB_KEY_BUFFER_SIZE;
                if (next != buffer_start) {
                    key_buffer[buffer_end] = (uint8)c;
                    buffer_end = next;
                }
            }
        }
        for (int i=0; i<8; i++) last_hid_report[i] = report[i];
    }
}

int usb_has_key(void) {
    usb_poll_direct();
    if (buffer_start != buffer_end) return 1;
    return (inb(0x64) & 1);
}

int usb_get_key(void) {
    usb_poll_direct();
    if (buffer_start != buffer_end) {
        int key = key_buffer[buffer_start];
        buffer_start = (buffer_start + 1) % USB_KEY_BUFFER_SIZE;
        return key;
    }
    // Fallback to legacy PS/2
    if (inb(0x64) & 1) {
        uint8 s = inb(0x60);
        if (s < 0x80) return scancode_table[s];
    }
    return 0;
}

void usb_keyboard_init(void) {
    usb_init_controller();
    usb_enumerate_hid_keyboard();
}
