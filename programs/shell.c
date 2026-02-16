#include "system.h"

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int program_main() {
    print("RTECH dos Shell v1.0 (OSx2)\n");
    char buffer[128];

    while (1) {
        input("> ", buffer, 128);

        if (strcmp(buffer, "exit") == 0) {
            print("Exiting...\n");
            exit();
            return 0;
        } else if (strcmp(buffer, "help") == 0) {
            print("Commands: devman, lsdev, format, mount, lsfs, testfs, help, exit\n");
        } else if (strcmp(buffer, "format") == 0) {
            format();
        } else if (strcmp(buffer, "mount") == 0) {
            mount();
        } else if (strcmp(buffer, "lsfs") == 0) {
            lsfs();
        } else if (strcmp(buffer, "testfs") == 0) {
            print("Creating file 'test'...\n");
            const char* data = "Hello RNAFS World!";
            UINTN len = 18;
            fwrite_sized("test", data, len);
            print("Reading back 'test'...\n");
            char read_buf[64];
            for(int i=0; i<64; i++) read_buf[i] = 0;
            if (fread("test", read_buf, 64) > 0) {
                print("Content: "); print(read_buf); print("\n");
            } else {
                print("Read failed.\n");
            }
        } else if (strcmp(buffer, "lsdev") == 0) {
            lsdev();
        } else if (strcmp(buffer, "devman") == 0) {
            devman();
        } else if (buffer[0]) {
            print("Bad command or file name: "); print(buffer); print("\n");
        }
    }
    return 0;
}
