#include "../include/system.h"
#include "../include/utils.h"

static void parse_and_execute(char* buffer);

static int strcmp_n(const char* s1, const char* s2, uint64 n) {
    for (uint64 i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == 0) {
            return 0;
        }
    }
    return 0;
}

static void cmd_help() {
    print("Commands: help, exit, echo [text], cat [file], write [file] [text]\n");
}

static void cmd_echo(char* arg) {
    print(arg);
    print("\n");
}

static void cmd_cat(char* filename) {
    char buf[1024];
    for (int i = 0; i < 1024; i++) {
        buf[i] = 0;
    }
    INTN bytes = fread(filename, buf, 1024);
    if (bytes >= 0) {
        print(buf);
        print("\n");
    } else {
        print("Error: Could not read file\n");
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
        uint64 len = 0;
        while (text[len]) {
            len++;
        }
        if (fwrite(filename, text, len) >= 0) {
            print("File written successfully\n");
        } else {
            print("Error: Could not write file\n");
        }
    } else {
        print("Usage: write [file] [text]\n");
    }
}

static void parse_and_execute(char* buffer) {
    if (strcmp(buffer, "help") == 0) {
        cmd_help();
    } else if (strcmp(buffer, "exit") == 0) {
        print("Exiting shell...\n");
        exit();
    } else if (strcmp_n(buffer, "echo ", 5) == 0) {
        cmd_echo(buffer + 5);
    } else if (strcmp_n(buffer, "cat ", 4) == 0) {
        cmd_cat(buffer + 4);
    } else if (strcmp_n(buffer, "write ", 6) == 0) {
        cmd_write(buffer + 6);
    } else if (buffer[0] != 0) {
        print("Unknown command: ");
        print(buffer);
        print("\n");
    }
}

int program_main() {
    print("OSx2 Advanced Shell v0.1\n");
    print("Type 'help' for commands\n");

    char buffer[128];
    while (1) {
        input("> ", buffer, 128);
        parse_and_execute(buffer);
    }

    return 0;
}
