#include "../include/rsl.h"

/*
 * OSx2 Standard Shell
 * Driving OSx2 (RTECH dos) via RSL (RTECH Standard Library).
 *
 * To add a new command, just add a new 'else if' in handle_command().
 * RSL handles the memory for you with Python-like ARC.
 */

static void handle_command(char* cmd);

int program_main() {
    print("OSx2 (RTECH dos) - Ready.\n");
    print("System Language: RSL (Python-like Memory)\n");

    while (1) {
        /* readline() automatically allocates memory for you. */
        char* input_str = readline("> ");

        if (input_str) {
            handle_command(input_str);

            /* Release the memory when done - ARC takes care of it. */
            release(input_str);
        }
    }

    return 0;
}

static void handle_command(char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        print("Commands: help, echo [text], cat [file], write [file] [text], clear, exit\n");
    } else if (strcmp(cmd, "exit") == 0) {
        print("System shutdown.\n");
        quit();
    } else if (strcmp(cmd, "clear") == 0) {
        clear();
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        print(cmd + 5);
        print("\n");
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        char* filename = cmd + 4;
        /* Using alloc() for a temporary buffer. */
        char* buf = (char*)alloc(1024);
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
    } else if (strncmp(cmd, "write ", 6) == 0) {
        char* arg = cmd + 6;
        char* text = strchr(arg, ' ');
        if (text) {
            *text = 0; /* Split filename and text */
            text++;
            if (write_file(arg, text, strlen(text)) >= 0) {
                print("File written successfully.\n");
            } else {
                print("Error: Could not write file.\n");
            }
        } else {
            print("Usage: write [file] [text]\n");
        }
    } else if (cmd[0] != 0) {
        print("Unknown command: ");
        print(cmd);
        print("\n");
    }
}
