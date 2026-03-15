# OSx2 (RTECH dos) Technical Implementation Blueprint (CODEBASE.md)

This document provides an exhaustive, low-level technical specification of the OSx2 codebase. It describes the implementation logic of every file with enough detail to allow for a complete system reconstruction.

---

## 1. Boot & Entry Layer (`/kernel/unice64`)

### `kernel/unice64/efi_entry.c` (Native UEFI Entry)
- **Purpose**: The "Native Diplomat". Serves as the PE32+ entry point for the firmware.
- **In-Code Logic**:
    - **Initialization**: Calls `InitializeLib` (gnu-efi) to setup the UEFI environment.
    - **Protocols**:
        - Locates `EFI_GRAPHICS_OUTPUT_PROTOCOL` (GOP) to extract the linear framebuffer address, resolution, and scanline width into a `boot_params_t` struct.
        - Handles `LOADED_IMAGE_PROTOCOL` and `SIMPLE_FILE_SYSTEM_PROTOCOL` to open the boot volume.
    - **File Loading**: Loads `shell.bin` from the ESP into a 48MB fixed memory location (`0x3000000`).
    - **Environment Prep**: Allocates memory for the ARC heap and the system ramdisk using `AllocatePages`.
    - **The Transition**: Calls `ExitBootServices` to terminate UEFI's control over the hardware, then immediately calls `kernel_main` (passing the populated `boot_params_t`).

---

## 2. Kernel Core Layer (`/kernel/unice64`)

### `kernel/unice64/main.c` (The Orchestrator)
- **Purpose**: Kernel initialization, service registry, and event loop.
- **In-Code Logic**:
    - **GDT Reset**: Calls `gdt_init` to re-establish segment descriptors for 64-bit mode after exiting UEFI services.
    - **Service Registry**: Iterates through `registered_services` (Console, Memory, Input, FS, etc.) and calls their init functions.
    - **Syscall Bridge**: Populates `rsl_syscall_table_t` with function pointers to kernel services.
    - **Execution**: Hands over control to the user-space shell by calling `loader_run_shell`.
    - **Event Loop**: Enters a `while(running)` loop that triggers `EVENT_MAIN`, allowing for asynchronous background processing.

### `kernel/unice64/gdt.c`
- **Purpose**: CPU segmentation for long mode.
- **In-Code Logic**: Defines a static GDT with Null, Code, and Data descriptors. Loads the GDT using the `lgdt` instruction to ensure the kernel runs in a controlled environment.

---

## 3. Kernel Services Layer (`/kernel/libs`)

### `kernel/libs/connect.c`
- **Purpose**: Physical hardware inventory. Maintains a registry of memory-mapped I/O and RAM regions.

### `kernel/libs/vdisk.c`
- **Purpose**: Storage virtualization layer. Translates virtual LBA requests to physical offsets on the ramdisk or other hardware.

### `kernel/libs/memory.c` (ARC System)
- **Purpose**: Reference-counted memory management.
- **In-Code Logic**: Implements a 'Free List' allocator on top of the UEFI-allocated heap. `alloc` returns ref-counted pointers; `release` decrements the count and returns the block to the free list if it hits zero.

### `kernel/libs/diskman.c`
- **Purpose**: Partition management. Parses GPT (GUID Partition Table) headers and manages VDISK partition mapping.

### `kernel/libs/rnafs.c` (Proprietary FS)
- **Purpose**: Contiguous filesystem on VDISKs.
- **In-Code Logic**: Uses a bitmap for block management. `rnafs_write` performs contiguous allocation and updates the directory entries. Fixed a critical bug where overwrites leaked blocks; it now deallocates old blocks before re-writing.

### `kernel/libs/console.c`
- **Purpose**: Graphics-mode text rendering. Draws 8x8 font characters directly to the GOP framebuffer.

### `kernel/libs/kutils.c` & `kutils.h`
- **Purpose**: Internal utility library. Uses `k_` prefixes (e.g., `k_memcpy`, `k_memset`) to avoid naming collisions with gnu-efi/UEFI symbols.

---

## 4. User Programs Layer (`/programs`)

### `programs/libsystem.c`
- **Purpose**: RTECH Standard Library (RSL) implementation for user-space. Caches the syscall table passed by the kernel.

### `programs/shell.c`
- **Purpose**: Native OSx2 Shell. Provides an interactive interface for file management and system commands.

---

## 5. Build System (`Makefile`)

- **Architecture**: Links all kernel components into a single `kernel.so` shared object, which is then converted to a `BOOTX64.EFI` application using `objcopy`.
- **Emulation**: Uses `qemu-system-x86_64` with the Q35 chipset and OVMF firmware to provide a modern UEFI boot environment.
