#include "../include/sys"

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
            print("Commands: help, exit\n");
        } else if (buffer[0]) {
            print("Bad command or file name: "); print(buffer); print("\n");
        }
    }
    return 0;
}
