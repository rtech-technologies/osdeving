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

## 4. Memory Model (ARC & Auto-RAM)
- OSx2 uses a Python-like transparent Automatic Reference Counting (ARC) system.
- Memory is managed via `auto_ram()`, `retain()`, and `release()`.
- RSL functions like `readline()` and `str_create()` automatically allocate RAM using this system.
- The kernel ensures RAM is handled safely without manual borrowing rules.

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
