# OSx2 (RTECH dos) Implementation Details (CODEBASE.md)

This document provides the exact technical implementation details of how the OSx2 system fulfills the requirements of `ARCHITECTURE.md`. It is intended for OS developers who need to understand or rewrite the system from scratch.

---

## 1. The Kernel Ritual & Service Registry (`main.c`)
To keep the kernel core (`main.c`) clean of hardware and filesystem logic, I implemented a **Service Registry** using function pointers.

- **Data Structure**: `static service_init_t registered_services[16]`.
- **Logic**:
  1. `register_service()` adds pointers to the initialization functions of each service (e.g., `console_init`, `memory_init`).
  2. In `kernel_start`, a for-loop iterates through `registered_services` and executes each one.
  3. This ensures that adding a new service only requires a single registration call in `main.c`, while the actual logic remains encapsulated in `/kernel/libs/`.

---

## 2. Two-Stage Handover & Signature Handshake (`loader/entry.c`)
The boot process transitions from a UEFI environment to a freestanding "God-Mode" kernel.

- **Stage 1 (UEFI Loader)**:
  - Allocates 16MB at `0x1000000` for the kernel binary.
  - Loads `kernel.bin` into that address.
  - **The Handshake**: It reads the first 4 bytes at `0x1000000`. If they do not match `0xDEADBEEF`, it halts. This prevents executing uninitialized memory.
  - **Jump**: It calls `ExitBootServices` and then performs a far jump to `0x1000004` (the instruction immediately following the signature).
- **Stage 2 (The Kernel)**:
  - Linked as a raw binary using `ld --oformat binary`.
  - The entry point `kernel_start` is pinned to the `.text.kernel_start_func` section, which the linker places immediately after the signature in the final binary.

---

## 3. ARC Memory Model Implementation (`kernel/libs/memory.c`)
The managed memory model is implemented using a prefix-header approach on a 32MB physical heap.

- **Header Structure**: Every allocation is preceded by an `arc_header_t` (16 bytes aligned):
  - `ref_count` (uint32): Number of active owners.
  - `size` (uint32): Allocated payload size.
  - `magic` (uint64): Set to `0x4152434D454D3031`.
- **Allocation Algorithm**:
  - Uses a **Bump Allocator**. A global `heap_ptr` tracks the next free byte.
  - `alloc(n)` adds `16 + n` to `heap_ptr` (aligned to 16 bytes).
- **Lifetime Management**:
  - `retain()` and `release()` verify the `magic` number before modifying the `ref_count`.
  - RSL functions like `readline()` automatically call `alloc()` and return the managed pointer to the user.

---

## 4. Connection Registry & VDISK Suit (`kernel/libs/connect.c`, `vdisk.c`)
To support diverse storage hardware, the kernel uses a virtualized I/O layer.

- **`/CONNECT` Registry**: `connect.c` tracks physical devices (DISK, RAM, USB). Each node defines its physical sector size and capacity.
- **VDISK Suit**: `vdisk.c` provides a Virtual Disk abstraction. It maps relative LBAs to physical offsets.
- **Handshake**: `vdisk_mount_verify()` enforces the `0xDEADBEEF` signature check at LBA 0 of every virtual disk before allowing access.
- **Error Handling**: Implements the "CANNOT FIND DISK" logic for disconnected hardware or signature mismatches.

---

## 5. GPT Compliance & RNAFS Virtualization (`diskman.c`, `rnafs.c`)
The high-level storage services now operate strictly through the VDISK layer.

- **GPT Compliance**: `diskman.c` manages GUID Partition Tables and recomputes CRC32 for headers and entry arrays. It creates VDISKs (e.g., `PART0`) for each partition.
- **RNAFS Logic**: `rnafs.c` implements bitmap-based contiguous allocation. Crucially, it no longer talks to physical memory; it performs all I/O via `vdisk_read` and `vdisk_write`, ensuring the same code works for RAM disks and future USB storage.

---

## 6. RSL Library & Syscall Abstraction (`programs/libsystem.c`)
User programs interact with the kernel through the **RTECH Standard Library (RSL)**.

- **Handover**: The kernel passes a `rsl_syscall_table_t` struct to the program's `_start` function.
- **Wrappers**: `libsystem.c` stores this table in a global pointer. Functions like `print()` and `read_file()` are simple wrappers that check if the pointer in the table is non-null and call it.
- **Independence**: This allows `shell.c` to be compiled without knowledge of kernel headers, strictly following the `ARCHITECTURE.md` rule.

---

## 7. Console Graphics & Font Rendering (`kernel/libs/console.c`)
OSx2 implements a custom text rendering engine on top of the UEFI GOP framebuffer.

- **Character Drawing**: `draw_char()` reads from a hardcoded 8x8 bitmap font (`font_data.c`). Each bit represents a pixel; if set, it writes the `fg_color` (RGB/BGR) to the corresponding `(x, y)` coordinate in the framebuffer.
- **Scrolling**: Implemented in `scroll()`. It uses `memcpy` to move every row of pixels up by a fixed amount (usually 10 pixels for the 8x8 font + 2px padding), then clears the bottom-most row with `bg_color`.
- **Theme Support**: The `console_init()` function checks for `CONFIG_EMERALD_MODE` to set the default neon-green color scheme.

---

## 8. PS/2 Keyboard Driver (`kernel/libs/input.c`)
A freestanding input driver that bypasses UEFI once the kernel takes control.

- **Polling Logic**: `input()` prompts the user and enters a loop that checks the PS/2 status register (Port 0x64).
- **Scancode Translation**: It reads the data register (Port 0x60) and uses a translation table to convert Set 1 scancodes into ASCII characters.
- **Interactive Echo**: The driver handles backspace (`\b`) by moving the console cursor back and overwriting the character with a space, providing a professional terminal experience.
