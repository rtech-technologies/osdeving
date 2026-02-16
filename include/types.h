#ifndef TYPES_H
#define TYPES_H

/* No stdint.h as per Second category */

typedef unsigned long long uint64;
typedef unsigned int       uint32;
typedef unsigned short     uint16;
typedef unsigned char      uint8;

typedef long long          int64;
typedef int               int32;
typedef short             int16;
typedef signed char       int8;

typedef uint64            size_t;
typedef uint64            uintptr_t;

typedef uint64            UINTN;
typedef int64             INTN;
typedef uint16            CHAR16;
typedef uint8             CHAR8;

typedef void*             EFI_HANDLE;
typedef UINTN             EFI_STATUS;

#define NULL ((void*)0)

#endif
