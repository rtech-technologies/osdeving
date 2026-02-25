#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <payload.bin> <out.rsl> [entry_offset] [init_offset]\n", argv[0]);
        return 2;
    }
    const char* in = argv[1];
    const char* out = argv[2];
    uint32_t entry = 20;
    uint32_t init = 0;
    if (argc >= 4) entry = (uint32_t)atoi(argv[3]);
    if (argc >= 5) init = (uint32_t)atoi(argv[4]);

    FILE* f = fopen(in, "rb");
    if (!f) { perror("open payload"); return 3; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len < 0) { fclose(f); fprintf(stderr, "failed to stat input\n"); return 4; }

    unsigned char* buf = malloc(len);
    if (!buf) { fclose(f); fprintf(stderr, "alloc failed\n"); return 5; }
    if (fread(buf, 1, len, f) != (size_t)len) { fclose(f); free(buf); fprintf(stderr, "read failed\n"); return 6; }
    fclose(f);

    FILE* o = fopen(out, "wb");
    if (!o) { perror("open out"); free(buf); return 7; }

    /* write header: magic + entry + init + image_size + reserved */
    unsigned char magic[4] = {'R','S','L','\0'};
    fwrite(magic, 1, 4, o);
    uint32_t hdr_size = 20;
    uint32_t image_size = hdr_size + (uint32_t)len;
    uint32_t reserved = 0;
    fwrite(&entry, sizeof(entry), 1, o);
    fwrite(&init, sizeof(init), 1, o);
    fwrite(&image_size, sizeof(image_size), 1, o);
    fwrite(&reserved, sizeof(reserved), 1, o);

    /* payload */
    if (fwrite(buf, 1, len, o) != (size_t)len) { fclose(o); free(buf); fprintf(stderr, "write payload failed\n"); return 8; }

    fclose(o);
    free(buf);
    return 0;
}
