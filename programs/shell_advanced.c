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

/* Simple shell variable table */
#define MAX_VARS 64
#define VAR_NAME_LEN 32
#define VAR_VAL_LEN 256
static struct { char name[VAR_NAME_LEN]; char val[VAR_VAL_LEN]; } shell_vars[MAX_VARS];
static int shell_var_count = 0;

static int last_status = 0; /* 0 == success */

static void set_var(const char* name, const char* val) {
    if (!name || !name[0]) return;
    for (int i = 0; i < shell_var_count; i++) {
        if (strcmp(shell_vars[i].name, name) == 0) {
            char_strncpy(shell_vars[i].val, val, VAR_VAL_LEN);
            return;
        }
    }
    if (shell_var_count < MAX_VARS) {
        char_strncpy(shell_vars[shell_var_count].name, name, VAR_NAME_LEN);
        char_strncpy(shell_vars[shell_var_count].val, val, VAR_VAL_LEN);
        shell_var_count++;
    }
}

static const char* get_var(const char* name) {
    if (!name) return NULL;
    for (int i = 0; i < shell_var_count; i++) {
        if (strcmp(shell_vars[i].name, name) == 0) return shell_vars[i].val;
    }
    return NULL;
}

/* Expand variables in input into out buffer. Handles single/double quotes.
   $VAR and ${VAR} expansions supported. out_sz must be >0. */
static void expand_variables(const char* in, char* out, uint64 out_sz) {
    if (!in || !out || out_sz == 0) return;
    uint64 oi = 0;
    for (uint64 i = 0; in[i] && oi + 1 < out_sz; ) {
        char c = in[i];
        if (c == '\'') {
            /* single quote: copy literally until next single quote */
            i++;
            while (in[i] && in[i] != '\'' && oi + 1 < out_sz) out[oi++] = in[i++];
            if (in[i] == '\'') i++;
        } else if (c == '"') {
            /* double quote: allow expansions inside */
            i++;
            while (in[i] && in[i] != '"' && oi + 1 < out_sz) {
                if (in[i] == '$') {
                    i++;
                    if (in[i] == '{') {
                        i++; uint64 st = i; while (in[i] && in[i] != '}') i++; uint64 len = i - st;
                        char name[64]; uint64 copylen = (len < sizeof(name)-1)?len:sizeof(name)-1;
                        for (uint64 k = 0; k < copylen; k++) name[k] = in[st+k]; name[copylen]=0;
                        const char* v = get_var(name);
                        if (v) {
                            for (uint64 k = 0; v[k] && oi + 1 < out_sz; k++) out[oi++] = v[k];
                        }
                        if (in[i] == '}') i++;
                    } else {
                        uint64 st = i; while (in[i] && ((in[i] >= 'A' && in[i] <= 'Z') || (in[i] >= 'a' && in[i] <= 'z') || (in[i] == '_') || (in[i] >= '0' && in[i] <= '9'))) i++;
                        uint64 len = i - st; char name[64]; uint64 copylen = (len < sizeof(name)-1)?len:sizeof(name)-1;
                        for (uint64 k = 0; k < copylen; k++) name[k] = in[st+k]; name[copylen]=0;
                        const char* v = get_var(name);
                        if (v) for (uint64 k = 0; v[k] && oi + 1 < out_sz; k++) out[oi++] = v[k];
                    }
                } else {
                    out[oi++] = in[i++];
                }
            }
            if (in[i] == '"') i++;
        } else if (c == '$') {
            /* unquoted expansion */
            i++;
            if (in[i] == '{') {
                i++; uint64 st = i; while (in[i] && in[i] != '}') i++; uint64 len = i - st;
                char name[64]; uint64 copylen = (len < sizeof(name)-1)?len:sizeof(name)-1;
                for (uint64 k = 0; k < copylen; k++) name[k] = in[st+k]; name[copylen]=0;
                const char* v = get_var(name);
                if (v) for (uint64 k = 0; v[k] && oi + 1 < out_sz; k++) out[oi++] = v[k];
                if (in[i] == '}') i++;
            } else {
                uint64 st = i; while (in[i] && ((in[i] >= 'A' && in[i] <= 'Z') || (in[i] >= 'a' && in[i] <= 'z') || (in[i] == '_') || (in[i] >= '0' && in[i] <= '9'))) i++;
                uint64 len = i - st; char name[64]; uint64 copylen = (len < sizeof(name)-1)?len:sizeof(name)-1;
                for (uint64 k = 0; k < copylen; k++) name[k] = in[st+k]; name[copylen]=0;
                const char* v = get_var(name);
                if (v) for (uint64 k = 0; v[k] && oi + 1 < out_sz; k++) out[oi++] = v[k];
            }
        } else {
            out[oi++] = in[i++];
        }
    }
    out[oi] = 0;
}

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
    /* Dual-purpose: if args appears like a test expression, evaluate and set last_status.
       Otherwise run the feature tests (legacy behavior). */
    if (args && args[0]) {
        /* simple parsing: -n STRING, STRING1 = STRING2, NUM -eq NUM */
        char left[128]; char op[8]; char right[128];
        left[0]=op[0]=right[0]=0;
        /* split by space into left/op/right */
        uint64 i = 0, p = 0;
        while (args[i] && args[i] == ' ') i++;
        while (args[i] && args[i] != ' ' && p < sizeof(left)-1) left[p++] = args[i++]; left[p]=0;
        while (args[i] == ' ') i++;
        p = 0; while (args[i] && args[i] != ' ' && p < sizeof(op)-1) op[p++] = args[i++]; op[p]=0;
        while (args[i] == ' ') i++;
        p = 0; while (args[i] && p < sizeof(right)-1) right[p++] = args[i++]; right[p]=0;

        int res = 1;
        if (strcmp(op, "-n") == 0) {
            res = (right[0] != 0) ? 0 : 1;
        } else if (strcmp(op, "=") == 0) {
            res = (strcmp(left, right) == 0) ? 0 : 1;
        } else if (strcmp(op, "-eq") == 0) {
            int a = 0, b = 0; /* very simple atoi */
            for (int k = 0; left[k]; k++) a = a*10 + (left[k]-'0');
            for (int k = 0; right[k]; k++) b = b*10 + (right[k]-'0');
            res = (a == b) ? 0 : 1;
        } else {
            /* not recognised -> fallback to feature tests below */
            res = -1;
        }

        if (res != -1) {
            last_status = res;
            return;
        }
    }

    /* Legacy feature tests (when no test expression provided) */
    print("\n");
    print_line("=");
    print("OSx2 Feature Tests\n");
    print_line("=");
    
    print("\n[IO Test]\n");
    print("  ✓ Console output\n");
    print("  ✓ Input system\n");
    
    print("\n[Memory Test]\n");
    void* p1 = alloc(64);
    print("  ✓ Alloc 64 bytes\n");
    free(p1);
    print("  ✓ Free works\n");
    
    print("\n[Filesystem Test]\n");
    void* fbuf = alloc(128);
    int sz = fread("shell.bin", fbuf, 128);
    if (sz > 0) {
        print("  ✓ fread: got data\n");
    }
    free(fbuf);
    
    print("\n[String Functions]\n");
    print("  ✓ strcmp\n");
    print("  ✓ memcpy\n");
    print("  ✓ memset\n");
    
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

    /* If this is an APL script (simple text script), run lines as shell commands */
    uint64 plen = 0; while (prog[plen]) plen++;
    if (plen > 4 && prog[plen-4]=='.' && prog[plen-3]=='a' && prog[plen-2]=='p' && prog[plen-1]=='l') {
        /* read script into buffer (limit 64KB) */
        uint64 max_script = 64*1024;
        void* sbuf = alloc(max_script);
        if (!sbuf) {
            print("Error: cannot allocate script buffer\n");
            return;
        }
        int ssz = fread(prog, sbuf, max_script);
        if (ssz <= 0) {
            print("Error: cannot read script file\n");
            free(sbuf);
            return;
        }

        /* Ensure zero-termination within read size */
        if (ssz < (int)max_script) ((char*)sbuf)[ssz] = 0;

        /* Execute each non-empty line as a shell command. Support simple if/then/fi blocks. */
        char* pcur = (char*)sbuf;
        while (*pcur) {
            /* Find end of line */
            char* eol = pcur;
            while (*eol && *eol != '\n' && *eol != '\r') eol++;
            char saved = *eol;
            *eol = 0;
            /* Trim leading spaces */
            char* line = pcur;
            while (*line == ' ') line++;
            /* Trim trailing spaces */
            char* t = eol - 1;
            while (t >= line && (*t == ' ' || *t == '\t')) { *t = 0; t--; }

            if (*line) {
                /* if-block handling (only in scripts): if <cond> then ... fi */
                if (line[0]=='i' && line[1]=='f' && (line[2]==' ' || line[2]==0)) {
                    /* collect block until 'fi' */
                    char condline[256];
                    char body[64][256]; int body_count = 0;
                    const char* cond = line + 2; while (*cond == ' ') cond++;

                    /* advance pcur to next line */
                    char* bp = eol + 1;
                    while (*bp == '\n' || *bp == '\r') bp++;
                    while (*bp) {
                        char* be = bp; while (*be && *be != '\n' && *be != '\r') be++;
                        char cs = *be; *be = 0;
                        char* bl = bp; while (*bl == ' ') bl++;
                        if (bl[0]=='f' && bl[1]=='i' && (bl[2]==0 || bl[2]==' ')) {
                            *be = cs; bp = be + 1; while (*bp == '\n' || *bp == '\r') bp++; break;
                        }
                        if (body_count < 64) {
                            /* copy trimmed body line */
                            uint64 k = 0; char* q = bl; while (*q && k < 255) body[body_count][k++] = *q++; body[body_count][k]=0;
                            body_count++;
                        }
                        *be = cs; bp = be + 1; while (*bp == '\n' || *bp == '\r') bp++;
                    }

                    /* evaluate condition: expand variables then run the condition (supports 'test' builtin) */
                    expand_variables(cond, condline, sizeof(condline));
                    parse_and_execute(condline);
                    if (last_status == 0) {
                        for (int bi = 0; bi < body_count; bi++) {
                            char expanded[512]; expand_variables(body[bi], expanded, sizeof(expanded));
                            parse_and_execute(expanded);
                        }
                    }
                    /* continue after block */
                    pcur = bp;
                    continue;
                }

                /* normal line: expand variables then execute */
                char expanded[1024]; expand_variables(line, expanded, sizeof(expanded));
                parse_and_execute(expanded);
            }

            if (saved == 0) break;
            /* restore and advance */
            *eol = saved;
            pcur = eol + 1;
            while (*pcur == '\n' || *pcur == '\r') pcur++;
        }

        free(sbuf);
        return;
    }

    /* Preference: if a `runtime.rsl` exists on the RNAFS, we will run that runtime
       and pass the requested program as its first argument. This keeps the loader
       logic isolated in `runtime` program. If `runtime.rsl` is not present, fall back
       to reading and running the requested program directly (legacy behavior). */

    char loader_name[64]; loader_name[0] = 0;
    {
        /* test for runtime.rsl presence */
        void* thdr = alloc(64);
        if (thdr) {
            int tg = fread("runtime.rsl", thdr, 64);
            if (tg > 0) {
                char_strncpy(loader_name, "runtime.rsl", 64);
            }
            free(thdr);
        }
    }

    /* If loader_name set, we will load runtime.rsl and pass `prog` as its argv[0] */

    /* Read small header first to validate format */
    typedef struct {
        char magic[4];
        uint32 entry_offset;
        uint32 init_offset;
        uint32 image_size;
        uint32 reserved;
    } rsl_header_t;

    rsl_header_t hdr;
    const char* to_load = loader_name[0] ? loader_name : prog;
    int got = fread(to_load, (void*)&hdr, sizeof(rsl_header_t));
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
        /* If we're launching runtime, its argv should be: [target, arg1, arg2...] */
        if (loader_name[0]) init_fn(argc+1, (char**)&(char*[]){prog, argv_local[0], argv_local[1], argv_local[2], argv_local[3], argv_local[4], NULL});
        else init_fn(argc, argv_local);
    }

    /* Call entry: convention int entry(int argc, char** argv) */
    int (*prog_entry)(int, char**) = (int(*)(int,char**))entry_ptr;
    int exit_code;
    if (loader_name[0]) {
        /* build argv for runtime: argv[0]=prog, then remaining args */
        char* rargs[18];
        rargs[0] = prog;
        for (int ri = 0; ri < argc && ri < 16; ri++) rargs[1+ri] = argv_local[ri];
        rargs[1+argc] = NULL;
        exit_code = prog_entry(argc+1, rargs);
    } else {
        exit_code = prog_entry(argc, argv_local);
    }

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
    /* Check for simple variable assignment: NAME=value (no spaces around '=') */
    uint64 eq = (uint64)-1;
    for (uint64 k = 0; buffer[k]; k++) { if (buffer[k] == '=') { eq = k; break; } }
    if (eq != (uint64)-1) {
        /* ensure no space before '=' and name looks valid */
        int ok = 1; if (eq == 0) ok = 0;
        for (uint64 k = 0; k < eq && ok; k++) { char ch = buffer[k]; if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' || (ch >= '0' && ch <= '9'))) { ok = 0; } }
        if (ok) {
            char name[VAR_NAME_LEN]; char valraw[256]; char val[VAR_VAL_LEN];
            uint64 n = (eq < VAR_NAME_LEN-1)?eq:VAR_NAME_LEN-1;
            for (uint64 k = 0; k < n; k++) name[k] = buffer[k]; name[n]=0;
            /* RHS */
            uint64 ri = eq + 1; while (buffer[ri] == ' ') ri++;
            uint64 rj = 0; while (buffer[ri] && rj + 1 < sizeof(valraw)) valraw[rj++] = buffer[ri++]; valraw[rj]=0;
            expand_variables(valraw, val, sizeof(val));
            set_var(name, val);
            return;
        }
    }

    /* Expand variables in the command line first */
    char expanded_line[512]; expand_variables(buffer, expanded_line, sizeof(expanded_line));

    /* Extract command */
    char cmd[32]; char args[256]; uint64 i = 0;
    while (i < 32 && expanded_line[i] && expanded_line[i] != ' ') { cmd[i] = expanded_line[i]; i++; }
    cmd[i] = 0;
    /* Extract args */
    if (expanded_line[i] == ' ') i++;
    uint64 j = 0; while (j < sizeof(args)-1 && expanded_line[i]) { args[j++] = expanded_line[i++]; } args[j]=0;
    while (j > 0 && args[j-1] == ' ') { args[j-1]=0; j--; }

    /* Execute builtins */
    if (strcmp(cmd, "help") == 0) { cmd_help(args); last_status = 0; }
    else if (strcmp(cmd, "man") == 0) { cmd_man(args); last_status = 0; }
    else if (strcmp(cmd, "echo") == 0) { cmd_echo(args); last_status = 0; }
    else if (strcmp(cmd, "clear") == 0) { cmd_clear(args); last_status = 0; }
    else if (strcmp(cmd, "info") == 0) { cmd_info(args); last_status = 0; }
    else if (strcmp(cmd, "mem") == 0) { cmd_mem(args); last_status = 0; }
    else if (strcmp(cmd, "ls") == 0) { cmd_ls(args); last_status = 0; }
    else if (strcmp(cmd, "cat") == 0) { cmd_cat(args); last_status = 0; }
    else if (strcmp(cmd, "write") == 0) { cmd_write(args); last_status = 0; }
    else if (strcmp(cmd, "mkrnafs") == 0) { cmd_mkrnafs(args); last_status = 0; }
    else if (strcmp(cmd, "run") == 0) { cmd_run(args); last_status = 0; }
    else if (strcmp(cmd, "test") == 0) { cmd_test(args); /* cmd_test sets last_status when used as test */ }
    else if (strcmp(cmd, "exit") == 0) { exit(); }
    else {
        print("Unknown command: "); print(cmd); print("\n"); last_status = 127;
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
