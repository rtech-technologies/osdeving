#ifndef EFI_TYPES_H
#define EFI_TYPES_H

#include "../include/types.h"

#define EFIAPI __attribute__((ms_abi))

#define EFI_SUCCESS 0
#define EFI_BUFFER_TOO_SMALL 0x8000000000000005ULL
#define EFI_INVALID_PARAMETER 0x8000000000000002ULL

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
typedef struct _EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;
typedef struct _EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

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
    uint16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_STATUS (EFIAPI *Reset)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, uint8 ExtendedVerification);
    EFI_STATUS (EFIAPI *ReadKeyStroke)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, EFI_INPUT_KEY *Key);
    void* WaitForKey;
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

struct _EFI_FILE_PROTOCOL {
    uint64 Revision;
    EFI_STATUS (EFIAPI *Open)(EFI_FILE_PROTOCOL *This, EFI_FILE_PROTOCOL **NewHandle, CHAR16 *FileName, uint64 OpenMode, uint64 Attributes);
    EFI_STATUS (EFIAPI *Close)(EFI_FILE_PROTOCOL *This);
    void* Delete;
    EFI_STATUS (EFIAPI *Read)(EFI_FILE_PROTOCOL *This, UINTN *BufferSize, void *Buffer);
    EFI_STATUS (EFIAPI *Write)(EFI_FILE_PROTOCOL *This, UINTN *BufferSize, void *Buffer);
    void* GetPosition;
    void* SetPosition;
    EFI_STATUS (EFIAPI *GetInfo)(EFI_FILE_PROTOCOL *This, EFI_GUID *InformationType, UINTN *BufferSize, void *Buffer);
};

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    uint64 Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This, EFI_FILE_PROTOCOL **Root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct {
    uint32 Revision;
    EFI_HANDLE ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE DeviceHandle;
    void *FilePath;
    void *Reserved;
    uint32 LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    uint64 ImageSize;
    uint32 ImageCodeType;
    uint32 ImageDataType;
    void *Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct {
    uint64 Size;
    uint64 FileSize;
    uint64 PhysicalSize;
    /* ... rest is timestamps and attributes */
} EFI_FILE_INFO;

struct _EFI_BOOT_SERVICES {
    uint8  Hdr[24];
    void*  RaiseTPL;
    void*  RestoreTPL;
    EFI_STATUS (EFIAPI *AllocatePages)(uint32 Type, uint32 MemoryType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory);
    EFI_STATUS (EFIAPI *FreePages)(EFI_PHYSICAL_ADDRESS Memory, UINTN Pages);
    EFI_STATUS (EFIAPI *GetMemoryMap)(UINTN *MemoryMapSize, void *MemoryMap, UINTN *MapKey, UINTN *DescriptorSize, uint32 *DescriptorVersion);
    EFI_STATUS (EFIAPI *AllocatePool)(uint32 PoolType, UINTN Size, void **Buffer);
    EFI_STATUS (EFIAPI *FreePool)(void *Buffer);
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
    void*  Reserved;
    void*  RegisterProtocolNotify;
    EFI_STATUS (EFIAPI *LocateHandle)(UINTN SearchType, EFI_GUID *Protocol, void *SearchKey, UINTN *BufferSize, EFI_HANDLE *Buffer);
    void*  LocateDevicePath;
    void*  InstallConfigurationTable;
    void*  LoadImage;
    void*  StartImage;
    void*  Exit;
    void*  UnloadImage;
    EFI_STATUS (EFIAPI *ExitBootServices)(EFI_HANDLE ImageHandle, UINTN MapKey);
    void*  GetNextMonotonicCount;
    void*  Stall;
    void*  SetWatchdogTimer;
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
    CHAR16 *FirmwareVendor;
    uint32 FirmwareRevision;
    uint32 Padding;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    void*  StdErr;
    void*  RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN  NumberOfTableEntries;
    void*  ConfigurationTable;
};

#endif
