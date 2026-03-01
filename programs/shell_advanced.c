#include "../include/rsl.h"

/* OSx2 Advanced Shell v0.3 - Powered by RSL with ARC */

static void parse_and_execute(char* buffer);

static void cmd_help() {
    print("Commands: help, exit, echo [text], cat [file], write [file] [text], meminfo\n");
}

static void cmd_echo(char* arg) {
    print(arg);
    print("\n");
}

static void cmd_cat(char* filename) {
    /* Demonstrate ARC: Allocate buffer */
    char* buf = (char*)alloc(1024);
    if (!buf) {
        print("Error: Allocation failed\n");
        return;
    }

    memset(buf, 0, 1024);
    INTN bytes = fread(filename, buf, 1024);
    if (bytes >= 0) {
        print(buf);
        print("\n");
    } else {
        print("Error: Could not read file\n");
    }

    /* Python-like: release when done */
    release(buf);
}

static void cmd_write(char* arg) {
    char* filename = arg;
    char* text = (void*)0;
    for (uint64 i = 0; arg[i]; i++) {
        if (arg[i] == ' ') {
            arg[i] = (char)0;
            text = &arg[i+1];
            break;
        }
    }

    if (text) {
        uint64 len = strlen(text);
        if (fwrite(filename, text, len) >= 0) {
            print("File written successfully\n");
        } else {
            print("Error: Could not write file\n");
        }
    } else {
        print("Usage: write [file] [text]\n");
    }
}

static void cmd_meminfo() {
    print("Memory Model: Python-like Transparent ARC\n");
    print("Heap: Managed via retain/release\n");
}

static void parse_and_execute(char* buffer) {
    if (strcmp(buffer, "help") == 0) {
        cmd_help();
    } else if (strcmp(buffer, "exit") == 0) {
        print("Exiting shell...\n");
        exit();
    } else if (strncmp(buffer, "echo ", 5) == 0) {
        cmd_echo(buffer + 5);
    } else if (strncmp(buffer, "cat ", 4) == 0) {
        cmd_cat(buffer + 4);
    } else if (strncmp(buffer, "write ", 6) == 0) {
        cmd_write(buffer + 6);
    } else if (strcmp(buffer, "meminfo") == 0) {
        cmd_meminfo();
    } else if (buffer[0] != 0) {
        print("Unknown command: ");
        print(buffer);
        print("\n");
    }
}

int program_main() {
    print("OSx2 Advanced Shell v0.3 (Pure RSL driven)\n");
    print("System Language: RSL\n");

    char buffer[128];
    while (1) {
        input("> ", buffer, 128);
        parse_and_execute(buffer);
    }

    return 0;
}
