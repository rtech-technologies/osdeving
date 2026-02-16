#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t  uint8;

typedef int64_t  int64;
typedef int32_t  int32;
typedef int16_t  int16;
typedef int8_t   int8;

typedef uint64_t UINTN;
typedef int64_t  INTN;
typedef uint16_t CHAR16;
typedef uint8_t  CHAR8;
typedef void*    EFI_HANDLE;
typedef UINTN    EFI_STATUS;

#define EFI_SUCCESS 0

#endif
