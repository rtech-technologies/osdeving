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

#ifndef __uintptr_t_defined
typedef unsigned long      uintptr_t;
#define __uintptr_t_defined
#endif

typedef unsigned long      size_t;

#if !defined(_EFI_H) && !defined(_EFI_BIND_H) && !defined(_EFIBIND_H_) && !defined(__EFI_TYPES_H__) && !defined(_EFI_TYPES_H) && !defined(X86_64_EFI_BIND) && !defined(_EFI_INCLUDE_)
typedef unsigned long     UINTN;
typedef long              INTN;
typedef uint16            CHAR16;
typedef uint8             CHAR8;

typedef void*             EFI_HANDLE;
typedef UINTN             EFI_STATUS;
typedef unsigned long     EFI_PHYSICAL_ADDRESS;
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#endif
