# OSx2 Kernel (RTECH dos)

A modular UEFI kernel built with a strict service-registry architecture.

## Project Structure

- `/kernel/unice64/`: Core kernel logic (UEFI entry, syscall dispatch).
- `/kernel/libs/`: Service libraries (Console, Memory, Disk, FS, Event, Init).
- `/kernel/libs/stup/`: Startup and main kernel loop.
- `/include/`: Public API headers. `include/sys.h` is the master header.
- `/programs/`: User programs (e.g., shell).
- `/boot/`: Linker scripts and boot configuration.

## Build Instructions

### Prerequisites
- `gnu-efi`
- `build-essential`
- `qemu-system-x86`
- `ovmf`
- `dosfstools`
- `mtools`

### Building
Run `make` in the root directory.

### Running
- `make run`: Run in QEMU with SDL graphical window and USB keyboard.
- `make run-serial`: Run in QEMU with nographic mode (Serial output).

## Architecture
- **Event-Driven**: The kernel uses an event system (EVENT_INIT, EVENT_MAIN, etc.).
- **Service Registry**: Services are registered during the INIT phase.
- **Unified API**: Programs use `system.h` and the `input()`/`print()` syscalls.
- **Multi-View Console**: Output is mirrored to both VGA framebuffer and Serial (COM1).
