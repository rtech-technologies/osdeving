# OSx2 (RTECH dos) Technical Specification (CODEBASE.md)

This document serves as the definitive technical reference for the OSx2 operating system architecture. It is designed to be used as a blueprint for a complete rewrite of the system.

## 1. Physical Memory Map

The system uses fixed physical memory offsets to ensure deterministic behavior between the UEFI Loader and the Freestanding Kernel.

| Base Address | Size | Description |
| :--- | :--- | :--- |
| `0x1000000` | 16 MB | **Stage 2 Kernel Binary.** First 4 bytes must be `0xDEADBEEF`. Entry point at `0x1000004`. |
| `0x2000000` | 8 MB | **Managed ARC Heap.** Allocated by Stage 1, used by Stage 2 for all `alloc()` calls. |
| `0x3000000` | 16 MB | **Program Memory.** Where `shell.bin` is loaded by Stage 1. |
| `0x4000000` | 16 MB | **System Ramdisk.** A dedicated, zeroed buffer used as the physical block device. |

---

## 2. Stage 1: The Handover Protocol (`loader/entry.c`)

The Stage 1 loader is a PE32+ UEFI application (`BOOTX64.EFI`) that performs the following sequence:

1.  **Hardware Discovery**: Locate `EFI_GRAPHICS_OUTPUT_PROTOCOL` (GOP) to retrieve the framebuffer base, resolution, and pixels-per-scanline.
2.  **Binary Loading**:
    - Load `kernel.bin` to `0x1000000` using `AllocatePages` with `Type 2` (EfiLoaderCode).
    - Load `shell.bin` to `0x3000000` (EfiLoaderCode).
3.  **Memory Setup**:
    - Allocate 8MB at `0x2000000` for the kernel heap.
    - Allocate 16MB at `0x4000000` for the system ramdisk and zero it out.
4.  **Mandatory Signature Handshake (Beef Check)**:
    - Read the `uint32` at `0x1000000`.
    - If `!= 0xDEADBEEF`, print "WHERES_THE_BEEF!" and halt via `hlt`.
5.  **Termination**: Call `ExitBootServices` to finalize the transition to freestanding mode.
6.  **Handover**: Execute the kernel by jumping to `0x1000004`, passing a pointer to the `boot_params_t` structure.

---

## 3. Stage 2: The Kernel Ritual (`kernel/unice64/main.c`)

The kernel architecture is strictly modular. `main.c` is the orchestrator and contains NO hardware logic.

### 3.1 Service Registry
A static array of initialization function pointers (`service_init_t`).
- Services like `console_init`, `memory_init`, `diskman_init`, etc., are registered here.
- The kernel loops through this array and calls each initialization function during the `EVENT_INIT` phase.

### 3.2 Event System
Defined in `kernel/libs/core/event.c`.
- Uses a `trigger(event_type)` mechanism to notify registered services of system state changes.
- **`EVENT_INIT`**: Bootstraps all services.
- **`EVENT_MAIN`**: The primary execution loop.
- **`EVENT_CLEANUP`** & **`EVENT_EXIT`**: Handles system termination.

---

## 4. Managed Memory: ARC Model (`kernel/libs/memory.c`)

OSx2 implements an **Automatic Reference Counting (ARC)** system. Every allocated pointer is prefixed with a 16-byte metadata header.

### 4.1 ARC Header Structure
```c
typedef struct {
    uint32 ref_count; // Number of active references
    uint32 size;      // Size of the payload
    uint64 magic;     // Must be 0x4152434D454D3031 ("ARCMEM01")
} arc_header_t;
```
- **`alloc(size)`**: Increments the `heap_ptr` by `sizeof(arc_header_t) + size`, aligns to 16 bytes, and initializes `ref_count` to 1.
- **`retain(ptr)`**: Increments `ref_count`.
- **`release(ptr)`**: Decrements `ref_count`. If zero, the magic is cleared. (v0 uses a bump allocator, so memory is not yet recycled).

---

## 5. Storage Stack: GPT & RNAFS (`kernel/libs/diskman.c`, `rnafs.c`)

### 5.1 GPT Partition Table
The `diskman` service manages the GUID Partition Table at LBA 1 of the ramdisk.
- **CRC32 Compliance**: After adding a partition, the service recomputes the `Partition Entry Array CRC` and the `GPT Header CRC`.
- **Bootstrapping**: If LBA 1 does not contain the "EFI PART" signature, `diskman` initializes a blank GPT header and partition entry array.

### 5.2 RNAFS v1 Specification
A proprietary, high-speed filesystem for OSx2.
- **Block Layout**:
    - Block 0: Superblock (`magic=0x5346414E52`, `data_start=6`).
    - Block 1: Allocation Bitmap (1 block).
    - Blocks 2-5: Directory Entries (16 max).
    - Blocks 6+: Data area.
- **Directory Entry (128 bytes, packed)**:
    - `name` (64s), `start_block` (Q), `size` (Q), `flags` (I), `padding` (44 bytes).
- **Allocation**: `rnafs_write` performs a contiguous block search in the bitmap before writing.

---

## 6. Console Graphics (`kernel/libs/console.c`)

The console renders directly to the GOP framebuffer (`32-bit BGR/RGB`).
- **Font Rendering**: Uses a manual 8x8 bitmap font (`font8x8_basic`).
- **Vertical Scrolling**: When `cursor_y + 10 > height`, the service performs a memory copy of the framebuffer:
  `memcpy(y, y + scroll_amount, pixels_per_scanline * 4)`.
- **Themes**: Supports `CONFIG_EMERALD_MODE` for high-contrast neon green text.

---

## 7. RSL Public API & Syscalls (`programs/libsystem.c`)

The **RTECH Standard Library (RSL)** is the interface for all user programs.
- **`rsl_syscall_table_t`**: A struct of function pointers passed from the kernel to the program entry point (`_start`).
- **`libsystem.c`**: Wraps these pointers into high-level C functions (e.g., `readline`, `str_create`, `lsfs`).
- **Program Entry**:
  ```c
  void _start(boot_params_t* params, rsl_syscall_table_t* syscalls);
  ```
  The program receives the hardware state and the kernel API upon execution.
