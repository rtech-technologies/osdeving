#include "../include/system.h"

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static uint32 atoi(const char* s) {
    uint32 res = 0;
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}

int program_main() {
    print("OSx2 Shell v0.3\n");
    char buffer[128];

    while (1) {
        input("> ", buffer, 128);

        if (strcmp(buffer, "exit") == 0) {
            print("Exiting...\n");
            exit();
            break;
        } else if (strcmp(buffer, "help") == 0) {
            print("Commands: help, exit, clear, format, mount, ls, addpart, testfs\n");
        } else if (strcmp(buffer, "format") == 0) {
            char s_idx[8];
            input("Partition index: ", s_idx, 8);
            format(atoi(s_idx));
        } else if (strcmp(buffer, "mount") == 0) {
            char s_idx[8];
            input("Partition index: ", s_idx, 8);
            mount(atoi(s_idx));
        } else if (strcmp(buffer, "ls") == 0) {
            lsfs();
        } else if (strcmp(buffer, "addpart") == 0) {
            char s_start[32], s_count[32];
            input("Start LBA: ", s_start, 32);
            input("Sector Count: ", s_count, 32);
            addpart(atoi(s_start), atoi(s_count));
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
                print("Write failed (Is RNAFS mounted?)\n");
            }
        } else if (buffer[0]) {
            print("Unknown command: ");
            print(buffer);
            print("\n");
        }
    }
    return 0;
}
