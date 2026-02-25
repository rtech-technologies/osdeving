#include "../include/system.h"
#include "../include/utils.h"

int program_main() {
    print("Shell started\n");
    print("Type 'exit' to quit kernel\n");

    char buffer[128];
    while (1) {
        input("> ", buffer, 128);

        if (strcmp(buffer, "exit") == 0) {
            print("Exiting shell...\n");
            exit();
            break;
        } else if (buffer[0] != 0) {
            print("Unknown command: ");
            print(buffer);
            print("\n");
        }
    }

    return 0;
}
