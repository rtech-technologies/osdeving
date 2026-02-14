#include "sys"

#define SHELL_BUFFER_SIZE 65536

void stup(boot_params_t* params) {
    // Everything is now inside init()
    init(params);
}

void main() {
    print("OSx2 (RTECH dos) Kernel Online\n");
    void* buffer = alloc(SHELL_BUFFER_SIZE);

    while (running) {
        if (fread("/shell.bin", buffer, SHELL_BUFFER_SIZE) > 0) {
            typedef int (*shell_t)(syscall_table_t*);
            ((shell_t)buffer)(&ksyscalls);
            if (running) {
                print("\nRestarting Shell...\n");
                for (volatile int i=0; i<20000000; i++);
            }
        } else {
            char cmd[64];
            input("os> ", cmd, 64);
            if (cmd[0]) {
                print("Bad command or file name: ");
                print(cmd);
                print("\n");
            }
        }
        trigger(EVENT_MAIN);
    }

    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
    print("System Halted.\n");
}
