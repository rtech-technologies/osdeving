# OSx2 (RTECH dos) Codebase Documentation

This document provides a comprehensive guide to the files and architecture of the OSx2 operating system.

## 1. High-Level Architecture (Two-Stage Handover)

OSx2 uses a professional two-stage boot process to ensure full "God-Mode" control over the hardware without relying on UEFI runtime services once the kernel is active.

1.  **Stage 1 (UEFI Loader):** A PE32+ application (`BOOTX64.EFI`) that runs under UEFI. Its job is to gather hardware information (GOP), allocate physical memory, load the Stage 2 kernel from disk, and perform a **Mandatory Signature Handshake** (`0xDEADBEEF`) before jumping into long mode.
2.  **Stage 2 (The Kernel):** A freestanding raw binary linked at `0x1000000`. It operates with zero UEFI dependencies, using its own drivers for graphics, keyboard, and disk access.

---

## 2. Directory Structure

- `/boot`: Linker scripts and UEFI-specific type definitions.
- `/include`: Public API headers (RSL) and kernel configuration.
- `/loader`: Source code for the Stage 1 UEFI loader.
- `/kernel/unice64`: Core kernel logic and the "Ritual" entry point.
- `/kernel/libs`: Modular kernel services and drivers.
- `/programs`: User-space programs (Shell) and the RSL library implementation.
- `/scripts`: Build and configuration utilities.
- `/tools`: Professional development tools (Font Studio).

---

## 3. Stage 1: The Loader (`/loader`)

### `loader/entry.c`
The entry point for the entire OS.
- **`efi_main`**: Initializes UEFI protocols, retrieves the Graphics Output Protocol (GOP) for the framebuffer, and opens the root volume.
- **File Loading**: Loads `kernel.bin` to `0x1000000` and `shell.bin` to `0x3000000`.
- **Memory Allocation**: Allocates a managed heap at `0x2000000` and a dedicated system ramdisk at `0x4000000`.
- **The Beef Check**: Verifies that the absolute top of the kernel binary contains the signature `0xDEADBEEF`. If it fails, the loader halts with a diagnostic error.
- **Handover**: Jumps to `0x1000004` (skipping the signature) to start the kernel.

---

## 4. Stage 2: The Kernel Core (`/kernel/unice64`)

### `kernel/unice64/main.c`
Implements the "Kernel Ritual" and service orchestration.
- **Service Registry**: Services register their initialization functions via `register_service()`.
- **`kernel_start`**: The Stage 2 entry point. It initializes all registered services in order (Console, Memory, Disk, etc.).
- **Event Loop**: Triggers `EVENT_INIT` and then enters the `EVENT_MAIN` loop while the system is running.
- **Syscall Table**: Populates the `rsl_syscall_table_t` which is passed to user programs.

### `kernel/unice64/gdt.c`
Sets up the Global Descriptor Table (GDT) for x86_64 long mode, ensuring proper segment descriptors for kernel code and data.

---

## 5. Kernel Services (`/kernel/libs`)

### `console.c` & `font_data.c`
The graphics driver. It renders text directly to the GOP framebuffer using an 8x8 bitmap font. Supports vertical scrolling and configurable themes (e.g., Emerald Mode).

### `memory.c`
Implements **Automatic Reference Counting (ARC)**.
- **`alloc(size)`**: Allocates a managed block with an `arc_header_t` containing the reference count and magic number.
- **`retain(ptr)` / `release(ptr)`**: Manages object lifetimes. In v0, this uses a high-speed bump allocator.

### `disk.c` & `diskman.c`
The storage stack.
- **`disk.c`**: Low-level driver for the dedicated ramdisk at `0x4000000`.
- **`diskman.c`**: Managed GPT (GUID Partition Table) compliance. It recomputes CRC32 for headers and entry arrays when partitions are modified.

### `rnafs.c`
The proprietary RNAFS driver.
- **Bitmap Allocation**: Uses a bitmap to track block usage, ensuring contiguous allocation for files.
- **Directory Management**: Handles packed 128-byte `rnafs_entry_t` structures.

### `input.c`
A freestanding PS/2 keyboard driver. It handles scancode-to-ASCII translation and provides the `input()` syscall for interactive command reading.

### `loader.c`
The Stage 2 program loader. It locates `shell.bin` on the RNAFS partition, loads it into memory, and executes it with a pointer to the kernel's syscall table.

### `kutils.c`
Standard kernel-level utilities like `memcpy`, `memset`, `itoa`, and the CRC32 implementation used for GPT compliance.

---

## 6. RSL & User Programs (`/include` & `/programs`)

### `include/rsl.h`
The **RTECH Standard Library (RSL)** public API. This is the *only* header programs should include. It defines high-level functions like `print()`, `readline()`, and `auto_ram()`.

### `programs/libsystem.c`
The glue between the RSL API and the kernel.
- **`_start`**: The program entry point. It receives the syscall table from the kernel and stores it for the RSL functions to use.
- **Syscall Wrappers**: Implements the RSL functions by calling the appropriate pointers in the syscall table.

### `programs/shell.c`
The OSx2 system shell. It provides a command-line interface for disk management (`format`, `addpart`, `mount`), filesystem operations (`lsfs`, `cat`, `write`), and system control.

---

## 7. Build & Development Tools (`/scripts` & `/tools`)

### `scripts/menuconfig.py`
A professional curses-based utility to configure the kernel (heap size, addresses, themes). It generates `.config` for the Makefile and `include/config.h` for the C compiler.

### `scripts/rnafs_tool.py`
A host-side tool for creating and manipulating RNAFS disk images.

### `tools/fontgen.html`
A web-based **Font Studio**. It allows developers to draw 8x8 glyphs and generate the C code used in `font_data.c`, while also previewing RGB color schemes.

### `Makefile`
The heart of the build system.
- **`make all`**: Builds the loader, kernel, and shell.
- **`make disk`**: Creates a bootable UEFI FAT32 image (`disk.img`) containing the loader and binaries.
- **`make run`**: Launches the system in QEMU with OVMF.
- **`make setup`**: Installs all required Linux dependencies for development.
