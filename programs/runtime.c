#include "../include/system.h"
#include "../include/utils.h"

/* Simple runtime program: argv[0] should be the target program path to load and execute.
   This program runs under the same userspace environment and uses fread/alloc/free syscalls. */

typedef struct {
    char magic[4];
    uint32 entry_offset;
    uint32 init_offset;
    uint32 image_size;
    uint32 reserved;
} rsl_header_t;

int entry(int argc, char** argv) {
    if (argc < 1 || !argv[0]) {
        print("Runtime: Usage: runtime <program.rsl>\n");
        return 1;
    }
    const char* target = argv[0];

    /* Read header */
    rsl_header_t hdr;
    int got = fread(target, &hdr, sizeof(hdr));
    if (got < (int)sizeof(hdr)) {
        print("Runtime: cannot read target header\n");
        return 2;
    }
    if (!(hdr.magic[0]=='R' && hdr.magic[1]=='S' && hdr.magic[2]=='L' && hdr.magic[3]=='\0')) {
        print("Runtime: invalid target format (expect RSL)\n");
        return 3;
    }

    if (hdr.image_size == 0) {
        print("Runtime: target reports zero image size\n");
        return 4;
    }

    void* img = alloc(hdr.image_size);
    if (!img) {
        print("Runtime: alloc failed\n");
        return 5;
    }

    int r = fread(target, img, hdr.image_size);
    if (r <= 0) {
        print("Runtime: failed to read target content\n");
        free(img);
        return 6;
    }

    /* Call init if present */
    if (hdr.init_offset != 0) {
        void (*init_fn)(int,char**) = (void(*)(int,char**))((char*)img + hdr.init_offset);
        init_fn(argc-1, argv+1);
    }

    int (*prog_entry)(int,char**) = (int(*)(int,char**))((char*)img + hdr.entry_offset);
    int rc = prog_entry(argc-1, argv+1);

    free(img);
    return rc;
}
