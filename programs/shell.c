#include "../include/system.h"

int program_main() {
    print("Shell started\n");
    print("Type 'exit' to quit kernel\n");

    /* For v0, we don't have input yet in this simplified version,
       so we'll just print and wait or exit immediately if we want to follow 'Nothing else'.
       But the user said 'Shell can print text and exit'.
    */

    print("Exiting shell...\n");
    exit();

    return 0;
}
