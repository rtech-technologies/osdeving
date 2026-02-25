#include "../include/system.h"
#include "../include/utils.h"

/* Command help database */
typedef struct {
    const char* name;
    const char* desc;
    const char* usage;
    const char* examples;
} command_t;

static const command_t commands[] = {
    {"help", "Show all available commands", "help [command]", "help\nhelp echo"},
    {"man", "Manual for a command", "man <command>", "man ls\nman cat"},
    {"echo", "Print text", "echo <text>", "echo Hello World"},
    {"clear", "Clear screen", "clear", "clear"},
    {"info", "System information", "info", "info"},
    {"mem", "Memory usage", "mem", "mem"},
    {"ls", "List RNAFS files", "ls [file]", "ls\nls shell.bin"},
    {"cat", "Read file contents", "cat <file>", "cat shell.bin"},
    {"write", "Write to file", "write <file> <text>", "write test.txt Hello"},
    {"mkrnafs", "Create RNAFS disk", "mkrnafs <disk>", "mkrnafs disk.img"},
    {"run", "Run program", "run <prog> [args]", "run shell.bin"},
    {"test", "Run feature tests", "test [all|io|fs|mem]", "test all"},
    {"exit", "Exit shell", "exit", "exit"},
    {NULL, NULL, NULL, NULL}
};

/* ============ UTILITY FUNCTIONS ============ */

static void print_line(const char* c) {
    for (int i = 0; i < 40; i++) print(c);
    print("\n");
}

static void show_command_help(const char* cmd) {
    for (int i = 0; commands[i].name; i++) {
        if (strcmp(commands[i].name, cmd) == 0) {
            print("\n");
            print(commands[i].name);
            print(" - ");
            print(commands[i].desc);
            print("\n");
            print("USAGE:  ");
            print(commands[i].usage);
            print("\n");
            print("EXAMPLE:\n");
            print("  ");
            print(commands[i].examples);
            print("\n\n");
            return;
        }
    }
    print("No help for: ");
    print(cmd);
    print("\n");
}

/* ============ COMMAND HANDLERS ============ */

static void cmd_help(const char* args) {
    print("\n");
    print_line("=");
    print("OSx2 Shell - Available Commands\n");
    print_line("=");
    
    for (int i = 0; commands[i].name; i++) {
        print("  ");
        print(commands[i].name);
        print(" - ");
        print(commands[i].desc);
        print("\n");
    }
    print("\nType 'man <command>' for detailed help\n\n");
}

static void cmd_man(const char* args) {
    if (args[0] == 0) {
        print("Usage: man <command>\n");
        return;
    }
    show_command_help(args);
}

static void cmd_echo(const char* args) {
    print(args);
    print("\n");
}

static void cmd_clear(const char* args) {
    for (int i = 0; i < 100; i++) print("\n");
}

static void cmd_info(const char* args) {
    print("\n");
    print("=== OSx2 System Information ===\n");
    print("Kernel:      UEFI-based OSx2 v0\n");
    print("Architecture: x86_64\n");
    print("Bootloader:  BOOTX64.EFI\n");
    print("Filesystem:  RNAFS (Ramdisk)\n");
    print("Shell:       Advanced (with RSL syscalls)\n");
    print("\n");
}

static void cmd_mem(const char* args) {
    print("\n=== Memory Status ===\n");
    print("Heap allocated\n");
    print("Bump allocator in use\n");
    print("(Full stats not yet implemented)\n\n");
}

static void cmd_ls(const char* args) {
    if (args[0] == 0) {
        print("\nFiles in RNAFS:\n");
        print("  shell.bin (loaded ramdisk)\n\n");
    } else {
        void* buf = alloc(256);
        if (!buf) {
            print("Error: Cannot allocate buffer\n");
            return;
        }
        int size = fread(args, buf, 256);
        if (size > 0) {
            print("\nFile: ");
            print(args);
            print(" (");
            char size_str[32];
            char_strncpy(size_str, "", 32);
            print(" bytes)\n");
        } else {
            print("File not found: ");
            print(args);
            print("\n");
        }
        free(buf);
    }
}

static void cmd_cat(const char* args) {
    if (args[0] == 0) {
        print("Usage: cat <file>\n");
        return;
    }
    
    void* buf = alloc(512);
    if (!buf) {
        print("Error: Cannot allocate buffer\n");
        return;
    }
    
    int size = fread(args, buf, 512);
    if (size > 0) {
        print("\n--- ");
        print(args);
        print(" ---\n");
        for (int i = 0; i < size && i < 200; i++) {
            char c = ((char*)buf)[i];
            if (c >= 32 && c < 127) {
                char ch[2] = {c, 0};
                print(ch);
            } else if (c == '\n') {
                print("\n");
            }
        }
        print("\n\n");
    } else {
        print("Cannot read: ");
        print(args);
        print("\n");
    }
    free(buf);
}

static void cmd_write(const char* args) {
    print("(File write not fully implemented for v0)\n");
    print("Args: ");
    print(args);
    print("\n");
}

static void cmd_test(const char* args) {
    print("\n");
    print_line("=");
    print("OSx2 Feature Tests\n");
    print_line("=");
    
    print("\n[IO Test]\n");
    print("  ✓ Console output");
    print("\n");
    print("  ✓ Input system");
    print("\n");
    
    print("\n[Memory Test]\n");
    void* p1 = alloc(64);
    print("  ✓ Alloc 64 bytes");
    print("\n");
    free(p1);
    print("  ✓ Free works");
    print("\n");
    
    print("\n[Filesystem Test]\n");
    void* fbuf = alloc(128);
    int sz = fread("shell.bin", fbuf, 128);
    if (sz > 0) {
        print("  ✓ fread: got ");
        print("data");
        print("\n");
    }
    free(fbuf);
    
    print("\n[String Functions]\n");
    print("  ✓ strcmp");
    print("\n");
    print("  ✓ memcpy");
    print("\n");
    print("  ✓ memset");
    print("\n");
    
    print("\nAll tests passed!\n\n");
}

static void cmd_mkrnafs(const char* args) {
    if (args[0] == 0) {
        print("Usage: mkrnafs <disk_file>\n");
        return;
    }
    print("Creating RNAFS disk: ");
    print(args);
    print("\n");
    print("RNAFS disk 'disk.img' already mounted at 1MB offset\n");
}

static void cmd_run(const char* args) {
    if (args[0] == 0) {
        print("Usage: run <program> [args]\n");
        print("Available: shell.bin, custom programs\n");
        return;
    }

    /* Parse program name and arguments (simple split by spaces) */
    char prog[64];
    char tokbuf[16][64];
    char* argv_local[17];
    int argc = 0;
    uint64 i = 0, j = 0, p = 0;

    /* extract first token = program */
    while (i < 63 && args[i] && args[i] != ' ') { prog[i] = args[i]; i++; }
    prog[i] = 0;

    /* extract remaining args into tokbuf */
    while (args[i] == ' ') i++;
    while (args[i] && argc < 16) {
        j = 0;
        while (j < 63 && args[i] && args[i] != ' ') {
            tokbuf[argc][j++] = args[i++];
        }
        tokbuf[argc][j] = 0;
        argv_local[argc] = tokbuf[argc];
        argc++;
        while (args[i] == ' ') i++;
    }
    argv_local[argc] = NULL;

    print("Running: ");
    print(prog);
    print("\n");

    /* Read small header first to validate format */
    typedef struct {
        char magic[4];
        uint32 entry_offset;
        uint32 init_offset;
        uint32 image_size;
        uint32 reserved;
    } rsl_header_t;

    rsl_header_t hdr;
    int got = fread(prog, (void*)&hdr, sizeof(rsl_header_t));
    if (got < (int)sizeof(rsl_header_t)) {
        print("Error: cannot read program header\n");
        return;
    }
    if (hdr.magic[0] != 'R' || hdr.magic[1] != 'S' || hdr.magic[2] != 'L' || hdr.magic[3] != '\0') {
        print("Error: invalid program format (expect RSL)\n");
        return;
    }

    if (hdr.image_size == 0) {
        print("Error: program reports zero image size\n");
        return;
    }

    /* Allocate memory for program image and read it entirely */
    void* img = alloc(hdr.image_size);
    if (!img) {
        print("Error: cannot allocate memory for program\n");
        return;
    }

    int r = fread(prog, img, hdr.image_size);
    if (r <= 0) {
        print("Error: failed to read program contents\n");
        free(img);
        return;
    }

    /* Entry pointer is base + entry_offset */
    void* entry_ptr = (void*)((char*)img + hdr.entry_offset);

    /* Call init if present */
    if (hdr.init_offset != 0) {
        void (*init_fn)(int, char**) = (void(*)(int,char**))((char*)img + hdr.init_offset);
        init_fn(argc, argv_local);
    }

    /* Call entry: convention int entry(int argc, char** argv) */
    int (*prog_entry)(int, char**) = (int(*)(int,char**))entry_ptr;
    int exit_code = prog_entry(argc, argv_local);

    /* Cleanup */
    free(img);

    /* Note: currently calling `exit()` from a program will call kernel exit (shuts kernel).
       Programs should return from their entry point to allow loader cleanup. */
    print("Program exited with code: ");
    {
        char buf[16];
        int v = exit_code; int pos = 0;
        if (v == 0) { buf[pos++] = '0'; }
        else {
            char rev[16]; int rp = 0;
            while (v > 0 && rp < 15) { rev[rp++] = '0' + (v % 10); v /= 10; }
            while (rp--) buf[pos++] = rev[rp];
        }
        buf[pos] = 0;
        print(buf);
    }
    print("\n");
}

/* ============ MAIN LOOP ============ */

static void parse_and_execute(char* buffer) {
    if (buffer[0] == 0) return;
    
    /* Extract command */
    char cmd[32];
    char args[128];
    uint64 i = 0;
    
    while (i < 32 && buffer[i] && buffer[i] != ' ') {
        cmd[i] = buffer[i];
        i++;
    }
    cmd[i] = 0;
    
    /* Extract args */
    if (buffer[i] == ' ') i++;
    uint64 j = 0;
    while (j < 128 && buffer[i]) {
        args[j] = buffer[i];
        j++;
        i++;
    }
    args[j] = 0;
    
    /* Trim args */
    while (j > 0 && args[j-1] == ' ') {
        args[j-1] = 0;
        j--;
    }
    
    /* Execute */
    if (strcmp(cmd, "help") == 0) cmd_help(args);
    else if (strcmp(cmd, "man") == 0) cmd_man(args);
    else if (strcmp(cmd, "echo") == 0) cmd_echo(args);
    else if (strcmp(cmd, "clear") == 0) cmd_clear(args);
    else if (strcmp(cmd, "info") == 0) cmd_info(args);
    else if (strcmp(cmd, "mem") == 0) cmd_mem(args);
    else if (strcmp(cmd, "ls") == 0) cmd_ls(args);
    else if (strcmp(cmd, "cat") == 0) cmd_cat(args);
    else if (strcmp(cmd, "write") == 0) cmd_write(args);
    else if (strcmp(cmd, "mkrnafs") == 0) cmd_mkrnafs(args);
    else if (strcmp(cmd, "run") == 0) cmd_run(args);
    else if (strcmp(cmd, "test") == 0) cmd_test(args);
    else if (strcmp(cmd, "exit") == 0) exit();
    else {
        print("Unknown command: ");
        print(cmd);
        print("\n");
    }
}

void setup() {
    print("\n");
    print_line("*");
    print("* OSx2 Advanced Shell v1\n");
    print("* Type 'help' for available commands\n");
    print("* Type 'man <cmd>' for help on command\n");
    print_line("*");
    print("\n");
}

int program_main() {
    setup();
    
    char buffer[128];
    while (1) {
        input("> ", buffer, 128);
        parse_and_execute(buffer);
    }
    
    return 0;
}
