#ifndef EFI_TYPES_H
#define EFI_TYPES_H

#include "../include/types.h"

#define EFIAPI __attribute__((ms_abi))

/* Category 12: use custom types */
#define EFI_SUCCESS 0

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef struct _EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;
typedef struct _EFI_BOOT_SERVICES EFI_BOOT_SERVICES;

typedef struct {
    uint32 Data1;
    uint16 Data2;
    uint16 Data3;
    uint8  Data4[8];
} EFI_GUID;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void* Reset;
    EFI_STATUS (EFIAPI *OutputString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);
    void* TestString;
    void* QueryMode;
    void* SetMode;
    void* SetAttribute;
    EFI_STATUS (EFIAPI *ClearScreen)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
};

typedef struct {
    uint32 HorizontalResolution;
    uint32 VerticalResolution;
    uint32 PixelFormat;
    uint32 PixelInformation[4];
    uint32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    uint32  MaxMode;
    uint32  Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN   SizeOfInfo;
    uint64  FrameBufferBase;
    UINTN   FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    void* QueryMode;
    void* SetMode;
    void* Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

struct _EFI_BOOT_SERVICES {
    uint8  Hdr[24];
    void*  RaiseTPL;
    void*  RestoreTPL;
    void*  AllocatePages;
    void*  FreePages;
    void*  GetMemoryMap;
    void*  AllocatePool;
    void*  FreePool;
    void*  CreateEvent;
    void*  SetTimer;
    void*  WaitForEvent;
    void*  SignalEvent;
    void*  CloseEvent;
    void*  CheckEvent;
    void*  InstallProtocolInterface;
    void*  ReinstallProtocolInterface;
    void*  UninstallProtocolInterface;
    EFI_STATUS (EFIAPI *HandleProtocol)(EFI_HANDLE Handle, EFI_GUID *Protocol, void **Interface);
    void*  RegisterProtocolNotify;
    EFI_STATUS (EFIAPI *LocateHandle)(UINTN SearchType, EFI_GUID *Protocol, void *SearchKey, UINTN *BufferSize, EFI_HANDLE *Buffer);
    void*  LocateDevicePath;
    void*  InstallConfigurationTable;
    void*  LoadImage;
    void*  StartImage;
    void*  Exit;
    void*  UnloadImage;
    EFI_STATUS (EFIAPI *ExitBootServices)(EFI_HANDLE ImageHandle, UINTN MapKey);
    // ...
    void*  GetNextMonotonicCount;
    void*  Stall;
    void*  SetWatchdogTimer;
    // ...
    void*  ConnectController;
    void*  DisconnectController;
    void*  OpenProtocol;
    void*  CloseProtocol;
    void*  OpenProtocolInformation;
    void*  ProtocolsPerHandle;
    void*  LocateHandleBuffer;
    EFI_STATUS (EFIAPI *LocateProtocol)(EFI_GUID *Protocol, void *Registration, void **Interface);
};

struct _EFI_SYSTEM_TABLE {
    uint8  Hdr[24];
    void*  ConsoleInHandle;
    void*  ConIn;
    void*  ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    void*  StandardErrorHandle;
    void*  StdErr;
    void*  RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
};

#endif
