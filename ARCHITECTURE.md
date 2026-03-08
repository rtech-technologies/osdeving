# OSx2 (RTECH dos) Architecture Rules

## 1. Directory Structure
- `/boot`: UEFI bootloader types and linker scripts.
- `/include`: Public headers and RSL (RTECH Standard Library) definitions.
- `/kernel/unice64`: Core kernel logic and entry points (mains).
- `/kernel/libs`: Internal kernel services and drivers.
- `/programs`: User-space programs (e.g., shell).

## 2. Kernel Lifecycle (The Ritual)
- `main.c` is strictly an orchestrator. It must NOT contain hardware, filesystem, or memory logic.
- Services are registered via `register_service(InitFunction)`.
- The kernel uses an event-driven flow:
  - `EVENT_INIT`: Service initialization and setup.
  - `EVENT_MAIN`: Main event loop execution.
  - `EVENT_CLEANUP`: Shutdown preparation.
  - `EVENT_EXIT`: Final termination.

## 3. RSL (RTECH Standard Library)
- RSL is the native system language for OSx2.
- Programs must include ONLY `<rsl.h>` and never internal kernel headers.
- High-level APIs like `readline()` and `color()` are provided for ease of use.

## 4. Memory Model (ARC & Managed RAM)
- OSx2 implements an Automatic Reference Counting (ARC) system for managed heap objects.
- High-level RSL functions (e.g., `readline`, `str_create`) return managed pointers with an initial ref-count of 1.
- Developers use `retain()` to increment and `release()` to decrement reference counts.
- **Note:** In the current v0.x implementation, the backing store is a high-speed bump allocator. While ref-counts are tracked, memory is not yet recycled for reuse. Manual `release()` calls are currently required for future-proofing and consistency with the RSL standard.

## 5. Storage Architecture (/CONNECT & VDISK)
- **Source of Truth**: The `/CONNECT` registry tracks all physical storage (Platters, RAM, USB).
- **Physical nodes**: Each device node has a configuration defining its Sector Size and Total LBA.
- **VDISK Suit**: A Virtual Disk abstraction layer that provides relative LBA access.
- **Signature Handshake**: VDISKs must contain the `0xDEADBEEF` signature at LBA 0 to be considered valid for mounting.
- **Error Handling**: Missing hardware or signature mismatches trigger a `CANNOT FIND DISK` or `Access Denied` error.

## 5. Console & Input
- Console supports full vertical scrolling and manual 8x8 font rendering to the GOP framebuffer.
- Advanced color selection is supported via `color(fg, bg)`.
- Input uses a freestanding PS/2 keyboard driver with Shift support.

## 6. Build Rules
- NO precompiled binaries or object files are allowed in the repository.
- The system must be buildable from pure source using the provided `Makefile`.
- Use `make menuconfig` to configure the kernel features and heap settings.

## 7. Development Tools
- `tools/fontgen.html`: A pro-grade web studio to draw fonts and preview colors.
- `scripts/menuconfig.py`: Terminal-based configuration utility (LFS-style).
- `scripts/rnafs_tool.py`: Host-side tool to manage RNAFS disk images and inject files.
