# OSx2 (RTECH dos) Technical Implementation Guide (CODEBASE.md)

This document provides a granular, file-by-file technical breakdown of the OSx2 codebase. It explains the purpose of each file and the specific code logic used to implement the system architecture.

---

## 1. Boot & Loader (`/loader`, `/boot`)

### `loader/entry.c` (Stage 1 Entry)
- **Purpose**: Transitions from UEFI firmware to the freestanding Stage 2 kernel.
- **Implementation**:
    - **Hardware Discovery**: Uses `ST->BootServices->LocateProtocol` to find `EFI_GRAPHICS_OUTPUT_PROTOCOL` (GOP). It stores the framebuffer base and resolution in a `boot_params_t` struct.
    - **Shell Preparation**: Loads `shell.bin` into memory at `0x3000000` and records the pointer in `boot_params_t` for the kernel.
    - **Binary Loading**: Opens the root FAT32 volume using `EFI_SIMPLE_FILE_SYSTEM_PROTOCOL`. It uses `AllocatePages` (Type 2, EfiLoaderCode) to reserve physical memory at `0x1000000` (Kernel) and `0x3000000` (Shell).
    - **Beef Check Handshake**: Before jumping, it reads `*(volatile uint32*)0x1000000`. If it is not `0xDEADBEEF`, it prints "WHERES_THE_BEEF!" and halts.
    - **Transition**: Calls `ExitBootServices` to reclaim firmware control and performs a far jump to `0x1000004`, passing the `boot_params_t` pointer.

### `boot/linker.ld` (Kernel Linker Script)
- **Purpose**: Defines the layout of the `kernel.bin` raw binary.
- **Implementation**:
    - Uses `ENTRY(kernel_start)` to define the entry point.
    - Sets the origin to `KERNEL_BASE` (passed via `--defsym` from the Makefile).
    - Uses `KEEP(*(.text.kernel_start))` to ensure the `0xDEADBEEF` signature is placed at offset 0.
    - Uses `KEEP(*(.text.kernel_start_func))` to ensure the entry function follows immediately at offset 4.

### `boot/efi_types.h` (UEFI Abstraction)
- **Purpose**: Minimal UEFI type definitions to avoid dependency on large headers like `efi.h`.
- **Implementation**: Defines basic UEFI structs (`EFI_SYSTEM_TABLE`, `EFI_BOOT_SERVICES`) and the `EFIAPI` calling convention (`ms_abi`).

---

## 2. Kernel Core (`/kernel/unice64`)

### `kernel/unice64/main.c` (Kernel Ritual)
- **Purpose**: Service orchestration and event loop.
- **Implementation**:
    - **Hardware Handover**: Immediately initializes a dedicated kernel stack and reloads the GDT to fully exit the UEFI environment.
    - **Service Registry**: Maintains a `registered_services` array. It uses `register_service(InitFunc)` to add initialization routines (Console, Memory, etc.).
    - **Initialization Loop**: In `kernel_start`, it iterates through the registry and executes each function.
    - **Event System**: Triggers `EVENT_INIT` to bootstrap services and enters a `while(running)` loop that triggers `EVENT_MAIN`.
    - **Syscall Handover**: Populates `rsl_syscall_table_t` with internal kernel function pointers and passes it to the program loader.

### `kernel/unice64/gdt.c` (Memory Segments)
- **Purpose**: Sets up the Global Descriptor Table for x86_64 long mode.
- **Implementation**: Defines three entries: Null, 64-bit Code (Access 0x9A, Granularity 0x20), and 64-bit Data (Access 0x92). It loads the table using the `lgdt` assembly instruction.

---

## 3. Kernel Services (`/kernel/libs`)

### `kernel/libs/connect.c` (Connection Registry)
- **Purpose**: The "Source of Truth" for storage hardware.
- **Implementation**:
    - **Registry**: Stores `physical_config_t` entries in a static array.
    - **RAM0 Registration**: Initializes `/CONNECT/RAM0/` using `kboot_params.ramdisk_base` and `ramdisk_size`.
    - **ARA Collision Detection**: Checks if `base_addr < 0x1000000` to alert of potential low-memory segment overlaps.

### `kernel/libs/vdisk.c` (The VDISK Suit)
- **Purpose**: Storage virtualization layer.
- **Implementation**:
    - **LBA Virtualization**: `vdisk_read/write` translate relative LBAs to physical addresses: `phys_lba = vd->start_lba + lba`.
    - **Signature Verification**: `vdisk_mount_verify` reads LBA 0 of the virtual disk. If the first 4 bytes aren't `0xDEADBEEF`, it returns "CANNOT FIND DISK" and denies access.

### `kernel/libs/memory.c` (ARC / ARA System)
- **Purpose**: Python-like Automatic Reference Counting.
- **Implementation**:
    - **Bump Allocator**: Increments a global `heap_ptr` by `sizeof(arc_header_t) + size`, aligned to 16 bytes.
    - **Ref-Counting**: `retain()` increments and `release()` decrements the `ref_count` in the `arc_header_t` prefix.
    - **Magic Check**: Validates `0x4152434D454D3031` before any ref-count modification to ensure pointer integrity.

### `kernel/libs/diskman.c` (GPT Compliance)
- **Purpose**: Managed GUID Partition Table compliance.
- **Implementation**:
    - **CRC32 Recomputation**: After adding a partition, it recomputes the GPT Header CRC (`header_crc = 0; crc32(header, size)`) and the Entry Array CRC.
    - **VDISK Creation**: Automatically opens a VDISK (e.g., `PART0`) for each partition found during init.

### `kernel/libs/rnafs.c` (Proprietary Filesystem)
- **Purpose**: Contiguous storage on VDISKs.
- **Implementation**:
    - **Bitmap Allocation**: `rnafs_write` scans the Block 1 bitmap for contiguous zero-bits. It uses a simple greedy algorithm to find space for the requested size.
    - **Directory Entries**: Writes 128-byte packed structs to Blocks 2-5.

### `kernel/libs/console.c` (Graphics Driver)
- **Purpose**: Text rendering to GOP framebuffer.
- **Implementation**:
    - **Scrolling**: Uses `memcpy` to shift the entire framebuffer up by 10 pixels: `memcpy(fb, fb + row_size * 10, row_size * (height - 10))`.
    - **Character Drawing**: Renders 8x8 glyphs from `font_data.c` by checking bits in the bitmap and writing `fg_color` to the corresponding framebuffer index.

### `kernel/libs/input.c` (Keyboard Driver)
- **Purpose**: Freestanding PS/2 input.
- **Implementation**: Polls Port `0x64` for status and reads scancodes from Port `0x60`. It uses a Set 1 lookup table for ASCII translation and manually handles backspace logic by overwriting characters with spaces.

---

## 4. RSL & User Programs (`/programs`, `/include`)

### `programs/libsystem.c` (RSL Core)
- **Purpose**: Implementation of the RTECH Standard Library.
- **Implementation**:
    - **Relocation Alignment**: Linked at `0x3000000` to match the Stage 1 memory handover.
    - **Bootstrapping**: Receives the `syscall_table` in `_start`.
    - **Managed Strings**: `str_create` calls `alloc()` and `strcpy()` to create ARC-managed strings.
    - **Interactive API**: `readline` uses the `input` syscall and returns a managed pointer.

### `programs/shell.c` (System Command UI)
- **Purpose**: User interface for OSx2.
- **Implementation**: Uses a `while(1)` loop calling `readline()`. Commands are parsed via `strcmp` and mapped to RSL functions like `format()`, `mount()`, and `lsfs()`.
