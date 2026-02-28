#include "../include/rsl.h"

/* OSx2 Advanced Shell v0.2 - Powered by RSL */

static void parse_and_execute(char* buffer);

static void cmd_help() {
    rsl_print("Commands: help, exit, echo [text], cat [file], write [file] [text], meminfo\n");
}

static void cmd_echo(char* arg) {
    rsl_print(arg);
    rsl_print("\n");
}

static void cmd_cat(char* filename) {
    char buf[1024];
    rsl_memset(buf, 0, 1024);
    INTN bytes = rsl_fread(filename, buf, 1024);
    if (bytes >= 0) {
        rsl_print(buf);
        rsl_print("\n");
    } else {
        rsl_print("Error: Could not read file\n");
    }
}

static void cmd_write(char* arg) {
    /* Simple write: find first space for filename */
    char* filename = arg;
    char* text = 0;
    for (uint64 i = 0; arg[i]; i++) {
        if (arg[i] == ' ') {
            arg[i] = 0;
            text = &arg[i+1];
            break;
        }
    }

    if (text) {
        uint64 len = rsl_strlen(text);
        if (rsl_fwrite(filename, text, len) >= 0) {
            rsl_print("File written successfully\n");
        } else {
            rsl_print("Error: Could not write file\n");
        }
    } else {
        rsl_print("Usage: write [file] [text]\n");
    }
}

static void cmd_meminfo() {
    /* For v0, we can't show actual RAM but we can show the heap base/size we got */
    rsl_print("Memory Info (Freestanding):\n");
    rsl_print("  Heap Base:  Available\n");
    rsl_print("  Heap Size:  4 MB\n");
}

static void parse_and_execute(char* buffer) {
    if (rsl_strcmp(buffer, "help") == 0) {
        cmd_help();
    } else if (rsl_strcmp(buffer, "exit") == 0) {
        rsl_print("Exiting shell...\n");
        rsl_exit();
    } else if (rsl_strncmp(buffer, "echo ", 5) == 0) {
        cmd_echo(buffer + 5);
    } else if (rsl_strncmp(buffer, "cat ", 4) == 0) {
        cmd_cat(buffer + 4);
    } else if (rsl_strncmp(buffer, "write ", 6) == 0) {
        cmd_write(buffer + 6);
    } else if (rsl_strcmp(buffer, "meminfo") == 0) {
        cmd_meminfo();
    } else if (buffer[0] != 0) {
        rsl_print("Unknown command: ");
        rsl_print(buffer);
        rsl_print("\n");
    }
}

int program_main() {
    rsl_print("OSx2 Advanced Shell v0.2 (RSL Driven)\n");
    rsl_print("Type 'help' for commands\n");

    char buffer[128];
    while (1) {
        rsl_input("> ", buffer, 128);
        parse_and_execute(buffer);
    }

    return 0;
}
