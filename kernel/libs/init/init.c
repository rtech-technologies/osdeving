#include "sys.h"

void init(boot_params_t* params) {
    kparams = params;

    // 1. Registry Ritual
    register_service(input_map_init);
    register_service(pci_init);
    register_service(devman_init);
    register_service(console_init);
    register_service(memory_init);
    register_service(disk_init);
    register_service(diskman_init);
    register_service(fs_init);
#ifdef CONFIG_USB_SUPPORT
    register_service(xhci_init);
    register_service(usb_keyboard_init);
#endif

    // 2. Event Setup
    event_init();
    register_event_handler(dispatch_init);
    register_event_handler(handle_exit);

    // 3. Syscall Table Population
    ksyscalls.print = print;
    ksyscalls.alloc = alloc;
    ksyscalls.free = free;
    ksyscalls.fread = fread;
    ksyscalls.fwrite = fwrite;
    ksyscalls.wait_for_key = wait_for_key;
    ksyscalls.read_key = read_key;
    ksyscalls.input = input;
    ksyscalls.lsdev = usb_lsdev;
    ksyscalls.devman = devman_show;
    ksyscalls.exit = kernel_exit;

    ksyscalls.format = diskman_format;
    ksyscalls.mount = diskman_mount;
    ksyscalls.lsfs = diskman_ls;
    ksyscalls.fwrite_sized = fs_write_sized;
    ksyscalls.fdelete = fs_delete;
    ksyscalls.addpart = diskman_add_partition;
    ksyscalls.mkfat = diskman_format_fat;

    // 4. System Initialization Event
    trigger(EVENT_INIT);
}
