#include "../include/system.h"

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int program_main() {
    print("OSx2 Shell v0.2\n");
    char buffer[128];

    while (1) {
        input("> ", buffer, 128);

        if (strcmp(buffer, "exit") == 0) {
            print("Exiting...\n");
            exit();
            break;
        } else if (strcmp(buffer, "help") == 0) {
            print("Commands: help, exit, clear, format, lsfs, testfs\n");
        } else if (strcmp(buffer, "clear") == 0) {
            print("Clear not implemented\n");
        } else if (strcmp(buffer, "format") == 0) {
            format();
        } else if (strcmp(buffer, "lsfs") == 0) {
            lsfs();
        } else if (strcmp(buffer, "testfs") == 0) {
            print("Creating file 'test'...\n");
            const char* data = "Hello RNAFS World!";
            if (fwrite("test", data, 18) > 0) {
                print("Read back test: ");
                char read_buf[32];
                for(int i=0; i<32; i++) read_buf[i] = 0;
                if (fread("test", read_buf, 32) > 0) {
                    print(read_buf);
                    print("\n");
                }
            } else {
                print("Write failed.\n");
            }
        } else if (buffer[0]) {
            print("Unknown command: ");
            print(buffer);
            print("\n");
        }
    }
    return 0;
}
