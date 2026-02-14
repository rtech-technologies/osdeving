#include "sys"

void init(boot_params_t* params) {
    kparams = params;

    // 1. Registry Ritual
    register_service(console_init);
    register_service(memory_init);
    register_service(disk_init);
    register_service(fs_init);

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
    ksyscalls.exit = kernel_exit;

    // 4. System Initialization Event
    trigger(EVENT_INIT);
}
