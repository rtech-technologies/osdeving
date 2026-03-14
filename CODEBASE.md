# OSx2 (RTECH dos) Technical Implementation Blueprint (CODEBASE.md)

This document provides an exhaustive, low-level technical specification of the OSx2 codebase. It describes the implementation logic of every file with enough detail to allow for a complete system reconstruction.

---

## 1. Boot & Handover Layer (`/loader`, `/boot`)

### `loader/entry.c` (UEFI Stage 1)
- **Purpose**: The "Diplomat". Initializes hardware and hands over to the Stage 2 "Dictator" (`os2.bin`).
- **In-Code Logic**:
    - **Branding**: Displays "OS*2 Loader: Locating Opaque Sheep...".
    - **Protocols**: Calls `LocateProtocol` for `EFI_GRAPHICS_OUTPUT_PROTOCOL` to extract `FrameBufferBase`, `HorizontalResolution`, and `PixelsPerScanLine` into a `boot_params_t` struct.
    - **Robust Error Handling**: Implements explicit "Sledgehammer" checks for every UEFI protocol handle and pointer. If `HandleProtocol`, `OpenVolume`, or `Open` fail, it prints a fatal error and halts to prevent NULL pointer dereferences (#PF).
    - **Storage**: Uses `LibFileInfo` and `AllocatePages` with `AllocateAddress` at `CONFIG_KERNEL_BASE` (default `0x100000`) to load `os2.bin`.
    - **Handshake**: Verifies the `0xDEADBEEF` signature at the kernel base. Prints "WHERES_THE_BEEF!" on mismatch.
    - **Exit**: Calls `ExitBootServices(ImageHandle, map_key)` to terminate UEFI environment control.
    - **Handover**: Executes a far jump by casting the entry address to a function pointer: `((void (*)(boot_params_t*))(CONFIG_KERNEL_BASE + 4))(params)`.

### `boot/linker.ld`
- **Purpose**: Defines the physical layout of the raw binary `os2.bin`.
- **In-Code Logic**:
    - `ENTRY(_start)`: Sets the entry symbol to the ASM stub.
    - `. = KERNEL_BASE`: Sets origin (default `0x100000`).
    - `KEEP(*(.text.kernel_start))`: Forces the `0xDEADBEEF` signature to the absolute first 4 bytes.
    - `*(.text)`: Places the ASM entry point immediately after.

### `kernel/unice64/entry.asm`
- **Purpose**: The machine's first freestanding instructions.
- **In-Code Logic**:
    - **CLI**: Clears interrupts to prevent UEFI legacy timer crashes.
    - **GDT Sledgehammer**: Loads the custom `rsl_gdt_pointer` and performs a `retfq` to flush the CS register to `0x08`.
    - **Stack**: Initializes a fresh 16KB stack in the `.bss` section.
    - **Transition**: Calls `kernel_main` (the C entry point), passing the `boot_params_t*` pointer.

### `boot/efi_types.h`
- **Purpose**: Minimal UEFI interface definition.
- **In-Code Logic**: Defines `EFI_SYSTEM_TABLE` and `EFI_BOOT_SERVICES` structs with exact UEFI-spec padding. Uses `__attribute__((ms_abi))` for all function pointers to ensure compatibility with firmware calling conventions.

---

## 2. Kernel Core Layer (`/kernel/unice64`)

### `kernel/unice64/main.c` (The Orchestrator)
- **Purpose**: Kernel entry, service registry, and event loop.
- **In-Code Logic**:
    - **Stack Reset**: Resets the stack pointer to the top of the kernel reservation area: `asm volatile ("mov %0, %%rsp" : : "r"(CONFIG_KERNEL_BASE + 16MB))`.
    - **Service Registry**: `static service_init_t registered_services[16]` stores init pointers.
    - **Handover**: Populates `rsl_syscall_table_t` with function pointers (e.g., `print`, `alloc`, `vdisk_read`) to be passed to user programs.
    - **Event Loop**: Triggers `EVENT_INIT`, then enters `while(running) { trigger(EVENT_MAIN); }`.

### `kernel/unice64/gdt.c`
- **Purpose**: CPU segmentation for long mode.
- **In-Code Logic**: Defines a static GDT with 3 entries: 0 (Null), 1 (Code: 0x9A access, 0x20 flags), and 2 (Data: 0x92 access). Loads via `lgdt` instruction.

### `kernel/unice64/io.h`
- **Purpose**: Hardware port primitives.
- **In-Code Logic**: Wraps `inb` and `outb` instructions using inline assembly with "a" (AL) and "Nd" (DX/Imm) constraints.

---

## 3. Kernel Services Layer (`/kernel/libs`)

### `kernel/libs/connect.c` & `connect.h`
- **Purpose**: Physical hardware inventory.
- **In-Code Logic**: Maintains `registry[8]` of `physical_config_t`. `connect_init` creates `/CONNECT/RAM0/` using the base/size provided by the loader. Includes overlap checks for the 1MB low-memory region.

### `kernel/libs/vdisk.c` & `vdisk.h`
- **Purpose**: Storage virtualization layer.
- **In-Code Logic**:
    - **LBA Mapping**: `phys_lba = vd->start_lba + lba`. Boundary checks against `vd->end_lba`.
    - **Security**: `vdisk_mount_verify` reads LBA 0; if the first 4 bytes are not `0xDEADBEEF`, it returns 0 (Access Denied).

### `kernel/libs/memory.c` & `memory.h` (ARC System)
- **Purpose**: Reference-counted memory management.
- **In-Code Logic**:
    - **Header**: Every block starts with `arc_header_t` (uint32 ref_count, uint32 size, uint64 magic).
    - **Allocation**: Bump-pointer `heap_ptr` increments by `(size + 16 + 15) & ~15` to ensure 16-byte alignment.
    - **Safety**: `release()` zeros the magic `0x4152434D454D3031` when the refcount hits zero.

### `kernel/libs/diskman.c` & `diskman.h`
- **Purpose**: GPT compliance and VDISK bridging.
- **In-Code Logic**:
    - **GPT Engine**: Reads LBA 1. If "EFI PART" matches, it creates VDISKs (e.g., "PART0") for each valid entry.
    - **CRC Engine**: Uses `crc32()` to recompute header (at offset 16) and entry array checksums after writes.

### `kernel/libs/rnafs.c` & `rnafs.h`
- **Purpose**: Contiguous filesystem on VDISKs.
- **In-Code Logic**:
    - **Bitmap**: Block 1 acts as a 4096-bit allocation map.
    - **Greedy Alloc**: `rnafs_write` scans bits in Block 1 for a contiguous run of 0s equal to the file's sector count.
    - **Directory**: Uses `__attribute__((packed))` on 128-byte `rnafs_entry_t` structs.

### `kernel/libs/console.c`, `font_data.c`, `font.h`
- **Purpose**: GOP-based graphics text rendering.
- **In-Code Logic**:
    - **Drawing**: `draw_char` bit-tests `font8x8_basic[c][row]`. If set, it writes the 32-bit color to `framebuffer[(y+row)*scanline + (x+col)]`.
    - **Scrolling**: `memcpy` shifts the framebuffer up by 10 rows.

### `kernel/libs/input.c` & `input.h`
- **Purpose**: Freestanding PS/2 input.
- **In-Code Logic**: Polls Port `0x64` (Status). If bit 0 is set, it reads the scancode from Port `0x60` and translates it using a Set 1 lookup table.

### `kernel/libs/kutils.c` & `kutils.h`
- **Purpose**: Internal utility library.
- **In-Code Logic**: Bitwise CRC32 (polynomial `0xEDB88320`), pointer-offset `memcpy`/`memset`, and base-10/16 `itoa`.

### `kernel/libs/loader.c` & `loader.h`
- **Purpose**: Stage 2 Handover and execution of the user shell.
- **In-Code Logic**:
    - **Execution**: Takes the `shell_base` (0x3000000) from `boot_params_t` and casts it to a function pointer: `void (*shell_entry)(boot_params_t*, rsl_syscall_table_t*)`.
    - **Syscall Bridge**: Calls this entry point passing a pointer to the `rsl_syscall_table_t` populated in `main.c`.
    - **Beef Diagnostic**: `debug_kernel_signature` (auditing `CONFIG_KERNEL_BASE`) is called here to ensure no late-stage memory corruption before handover.

### `kernel/libs/core/event.c` & `event.h`
- **Purpose**: Asynchronous-style event notification system.
- **In-Code Logic**:
    - **Storage**: `static event_handler_t handlers[32]` array of function pointers.
    - **Trigger**: `trigger(event_t event)` iterates from `0` to `handler_count - 1` and executes `handlers[i](event)`. This allows services to react to system lifecycle events (INIT, MAIN, CLEANUP) without tight coupling.

---

## 4. User Programs Layer (`/programs`, `/include`)

### `programs/libsystem.c` & `include/rsl.h`
- **Purpose**: RTECH Standard Library implementation.
- **In-Code Logic**: Caches the kernel's `rsl_syscall_table_t`. High-level functions (e.g., `readline`) check if the kernel pointer is valid before calling.

### `programs/shell.c`
- **Purpose**: Interactive user shell.
- **In-Code Logic**: `while(1)` loop calling `readline()`. Uses `strncmp` to route commands to RSL storage functions.

### `programs/linker.ld`
- **Purpose**: User-space linking.
- **In-Code Logic**: Sets origin to `0x3000000` to match Stage 1 handover.

---

## 5. System Definitions (`/include`)

### `include/types.h`
- **Purpose**: Portable freestanding types.
- **In-Code Logic**: Defines `uint8`, `uint16`, `uint32`, `uint64`, `INTN`, and `size_t`.

### `include/config.h`
- **Purpose**: Build settings.
- **In-Code Logic**: Auto-generated by `menuconfig.py`.

---

## 6. Development Tools (`/scripts`, `/tools`, `Makefile`)

### `scripts/menuconfig.py`
- **Purpose**: Kernel configuration UI.
- **In-Code Logic**: Uses `curses` to manage `.config`. Automatically generates `include/config.h`.

### `scripts/rnafs_tool.py`
- **Purpose**: Disk image builder.
- **In-Code Logic**: Implements RNAFS bitmap and directory logic in Python to inject files.

### `Makefile`
- **Purpose**: System architect.
- **In-Code Logic**:
    - EFI: `-fpic -fshort-wchar -shared -Bsymbolic`.
    - Kernel: `-ffreestanding -nostdlib --oformat binary --defsym=KERNEL_BASE=...`.
    - Image: `mtools` and `xorriso` for ISO generation.
