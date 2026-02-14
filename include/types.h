#ifndef TYPES_H
#define TYPES_H

#include <efi.h>

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef UINT64             uint64;

typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;
typedef INT64              int64;

typedef UINTN              size_t;
typedef INTN               ssize_t;

typedef struct {
    uint32* framebuffer;
    uint32  width;
    uint32  height;
    uint32  pixels_per_scanline;
    EFI_SYSTEM_TABLE* st;
} boot_params_t;

#endif
