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
- All kernel services are exposed to programs via a unified `rsl_syscall_table_t`.

## 4. Memory Model (ARC)
- OSx2 uses a Python-like transparent Automatic Reference Counting (ARC) system.
- Memory is managed via `alloc`, `retain`, and `release`.
- Manual borrowing or lifetime management should be minimized in high-level RSL code.

## 5. Console & Input
- Console supports full vertical scrolling and manual 8x8 font rendering to the GOP framebuffer.
- Input uses a freestanding PS/2 keyboard driver with Shift support.
- Hardware interaction is strictly confined to services in `/kernel/libs`.

## 6. Build Rules
- NO precompiled binaries or object files are allowed in the repository.
- The system must be buildable from pure source using the provided `Makefile`.
- `make all` produces the UEFI-bootable `BOOTX64.EFI` and the shell binary.

## 7. Development Tools
- `tools/fontgen.html`: A web-based utility to generate 8x8 font data for the console. Characters are stored as 8 bytes, where each byte represents a row and bits represent columns (bit 0 = left).
