#include "../include/rsl.h"

/*
 * OSx2 Standard Shell
 * Powering RTECH dos via the RSL (RTECH Standard Library).
 *
 * This shell is designed to be easy for experts and noobs.
 * Add new commands in handle_command().
 */

static void handle_command(char* cmd);

int program_main() {
    clear();
    color(0x00FF88, 0x000000); /* Pro Emerald Green */
    print("OSx2 (RTECH dos) - Kernel Expert Mode\n");
    print("RSL Standard Library v1.0 - ARC Memory Enabled\n\n");

    while (1) {
        /* Auto-RAM: readline handles allocation automatically */
        char* input_str = readline("> ");

        if (input_str) {
            handle_command(input_str);

            /* Manual Release (ARC): optional for experts,
               but good practice for large strings. */
            release(input_str);
        }
    }

    return 0;
}

static void handle_command(char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        print("Commands: help, echo [text], cat [file], write [file] [text], color [hex], clear, exit\n");
    } else if (strcmp(cmd, "exit") == 0) {
        print("System shutdown requested.\n");
        quit();
    } else if (strcmp(cmd, "clear") == 0) {
        clear();
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        print(cmd + 5);
        print("\n");
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        char* filename = cmd + 4;
        /* Using auto_ram for a temporary buffer */
        char* buf = (char*)auto_ram(1024);
        if (buf) {
            memset(buf, 0, 1024);
            if (read_file(filename, buf, 1024) >= 0) {
                print(buf);
                print("\n");
            } else {
                print("Error: File not found.\n");
            }
            release(buf);
        }
    } else if (strncmp(cmd, "color ", 6) == 0) {
        /* Reset to standard emerald green */
        color(0x00FF88, 0x000000);
        print("Colors reset to Emerald Green.\n");
    } else if (cmd[0] != 0) {
        print("Unknown command: ");
        print(cmd);
        print("\n");
    }
}
