#include "system.h"

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

uint32 atoi(const char* s) {
    uint32 res = 0;
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
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
            print("Commands: devman, lsdev, format, mount, lsfs, testfs, del, addpart, mkfat, reboot, shutdown, help, exit\n");
        } else if (strcmp(buffer, "format") == 0) {
            format();
        } else if (strcmp(buffer, "mount") == 0) {
            char s_idx[8];
            input("partition index: ", s_idx, 8);
            mount(atoi(s_idx));
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
        } else if (strcmp(buffer, "del") == 0) {
            char path[64];
            input("filename: ", path, 64);
            if (fdelete(path) == 0) {
                print("Deleted.\n");
            } else {
                print("Delete failed.\n");
            }
        } else if (strcmp(buffer, "addpart") == 0) {
            char s_start[32], s_count[32];
            input("start lba: ", s_start, 32);
            input("sector count: ", s_count, 32);
            addpart(atoi(s_start), atoi(s_count));
        } else if (strcmp(buffer, "mkfat") == 0) {
            char s_idx[8];
            input("partition index: ", s_idx, 8);
            mkfat(atoi(s_idx));
        } else if (strcmp(buffer, "reboot") == 0) {
            reboot();
        } else if (strcmp(buffer, "shutdown") == 0) {
            shutdown();
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
