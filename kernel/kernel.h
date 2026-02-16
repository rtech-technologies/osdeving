#ifndef KERNEL_H
#define KERNEL_H

#include "../include/types.h"
#include "../include/system.h"

#define EFIAPI __attribute__((ms_abi))

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef struct _EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;

typedef struct _EFI_FILE_PROTOCOL {
    UINTN Revision;
    EFI_STATUS (EFIAPI *Open)(struct _EFI_FILE_PROTOCOL *This, struct _EFI_FILE_PROTOCOL **NewHandle, CHAR16 *FileName, uint64 OpenMode, uint64 Attributes);
    EFI_STATUS (EFIAPI *Close)(struct _EFI_FILE_PROTOCOL *This);
    void* Delete;
    EFI_STATUS (EFIAPI *Read)(struct _EFI_FILE_PROTOCOL *This, UINTN *BufferSize, void *Buffer);
    // ... rest of it
} EFI_FILE_PROTOCOL;

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINTN Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This, EFI_FILE_PROTOCOL **Root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct {
    uint32 Data1;
    uint16 Data2;
    uint16 Data3;
    uint8 Data4[8];
} EFI_GUID;

typedef struct _EFI_BOOT_SERVICES {
    char Hdr[24];
    void* RaiseTPL;
    void* RestoreTPL;
    void* AllocatePages;
    void* FreePages;
    void* GetMemoryMap;
    void* AllocatePool;
    void* FreePool;
    void* CreateEvent;
    void* SetTimer;
    void* WaitForEvent;
    void* SignalEvent;
    void* CloseEvent;
    void* CheckEvent;
    void* InstallProtocolInterface;
    void* ReinstallProtocolInterface;
    void* UninstallProtocolInterface;
    EFI_STATUS (EFIAPI *HandleProtocol)(EFI_HANDLE Handle, EFI_GUID *Protocol, void **Interface);
    // ...
} EFI_BOOT_SERVICES;

typedef struct _EFI_LOADED_IMAGE_PROTOCOL {
    UINTN Revision;
    EFI_HANDLE ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE DeviceHandle;
    // ...
} EFI_LOADED_IMAGE_PROTOCOL;

// UEFI Minimal Definitions
struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void* Reset;
    EFI_STATUS (EFIAPI *OutputString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);
    void* TestString;
    void* QueryMode;
    void* SetMode;
    void* SetAttribute;
    EFI_STATUS (EFIAPI *ClearScreen)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
};

struct _EFI_SYSTEM_TABLE {
    uint8 Header[24];
    EFI_HANDLE ConsoleInHandle;
    void* ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    void* StandardErrorHandle;
    void* StdErr;
    void* RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    void* ConfigurationTable;
};

// Events
typedef enum {
    EVENT_INIT,
    EVENT_MAIN,
    EVENT_CLEANUP,
    EVENT_EXIT
} event_t;

// Service Registry
typedef void (*service_init_t)();
void register_service(service_init_t init_func);

// Event System
void trigger(event_t event);
typedef void (*event_handler_t)(event_t event);
void register_event_handler(event_handler_t handler);

// Global State
extern EFI_SYSTEM_TABLE *ST_PTR;
extern EFI_HANDLE ImageHandle_PTR;
extern int running;

#endif
