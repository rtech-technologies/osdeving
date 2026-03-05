#include "../include/rsl.h"

/*
 * OSx2 Standard Shell
 * Powering RTECH dos via the RSL (RTECH Standard Library).
 */

static void handle_command(char* cmd);

int program_main() {
    clear();
    color(0x00FF88, 0x000000);
    print("OSx2 (RTECH dos) - Kernel Expert Mode (64-bit)\n");
    print("Partition Table: GPT (GUID Partition Table)\n");
    print("Permanent Storage: RNAFS (Proprietary)\n\n");

    while (1) {
        char* input_str = readline("> ");
        if (input_str) {
            handle_command(input_str);
            release(input_str);
        }
    }
    return 0;
}

static void handle_command(char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        print("Commands:\n");
        print("  lsfs          - List files on mounted RNAFS\n");
        print("  cat [file]    - Read file content\n");
        print("  write [f] [t] - Write text to file\n");
        print("  addpart [s] [c]- Add GPT partition (Start LBA, Count)\n");
        print("  format [idx]  - Format GPT partition with RNAFS\n");
        print("  mount [idx]   - Mount RNAFS partition\n");
        print("  clear         - Clear screen\n");
        print("  exit          - Shutdown\n");
    } else if (strcmp(cmd, "exit") == 0) {
        quit();
    } else if (strcmp(cmd, "clear") == 0) {
        clear();
    } else if (strncmp(cmd, "addpart ", 8) == 0) {
        char* start_str = cmd + 8;
        char* count_str = strchr(start_str, ' ');
        if (start_str && count_str) {
            *count_str = 0;
            count_str++;
            addpart(atoi(start_str), atoi(count_str));
        }
    } else if (strncmp(cmd, "format ", 7) == 0) {
        format(atoi(cmd + 7));
    } else if (strncmp(cmd, "mount ", 6) == 0) {
        mount(atoi(cmd + 6));
    } else if (strcmp(cmd, "lsfs") == 0) {
        lsfs();
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        char* buf = (char*)auto_ram(4096);
        if (buf) {
            memset(buf, 0, 4096);
            if (read_file(cmd + 4, buf, 4096) >= 0) {
                print(buf);
                print("\n");
            } else {
                print("Error: File not found or RNAFS not mounted.\n");
            }
            release(buf);
        }
    } else if (strncmp(cmd, "write ", 6) == 0) {
        char* arg = cmd + 6;
        char* text = strchr(arg, ' ');
        if (text) {
            *text = 0;
            text++;
            if (write_file(arg, text, strlen(text)) >= 0) {
                print("Write successful.\n");
            } else {
                print("Write failed.\n");
            }
        }
    } else if (cmd[0] != 0) {
        print("Unknown command: ");
        print(cmd);
        print("\n");
    }
}
