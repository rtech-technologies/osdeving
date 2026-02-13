#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>

// Use standard types that match UEFI's UINTN on 64-bit
typedef uint64_t UINTN;
typedef int64_t  INTN;

typedef intptr_t intptr;
typedef uintptr_t uintptr;

#endif
