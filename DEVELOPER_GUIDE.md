DEVELOPER GUIDE — OSx2 (osdeving)

Purpose
- Short, practical guide for contributors and active development.
- Focus: building, testing, running, adding userspace programs, and implementing the loader/mkrnafs work.

Repository layout (high level)
- Makefile               — build orchestration (menuconfig, set, make, run)
- boot/                  — UEFI boot sources and linker
- include/               — shared headers (`system.h`, `types.h`, `utils.h`)
- kernel/                — kernel entry, main and kernel-level services
- services/              — kernel services (console, fs, memory, input, rnafs, core/event)
- programs/              — userspace programs (`libsystem.c`, `shell_advanced.c`, others)
- setup.sh               — interactive configuration helper

Quickstart (local dev)
1. Ensure required tools installed: `make`, `gcc` (cross or native used by Makefile), `qemu-system-x86_64`, `nasm` (if required), `wget`, `tar`.
2. In project root run:

```bash
chmod +x setup.sh
./setup.sh set   # or `make set` depending on preference
make             # build kernel and userspace
make run         # launches QEMU with built image
```

Makefile targets
- `make set` / `./setup.sh set` — interactive or scripted config
- `make` — build kernel, services and userspace
- `make run` — run in QEMU (depends on .config)
- `make clean` — clean build artifacts

Development workflow notes
- Keep kernel/boot changes minimal unless necessary — userspace is safer for iteration.
- `programs/shell_advanced.c` is primary development surface for userspace features and loader logic.
- Use `include/utils.h` helpers for bounds-checked `memcpy`/`memset`/`strncpy`.

RSL syscall contract (userspace/kernel interface)
- The kernel exposes a fixed syscall table (7 syscalls). Userspace programs must use `libsystem.c` wrapper functions to call syscalls.
- Do not change syscall ordering without updating `kernel/main.c` and `libsystem.c` consistently.

RNAFS (ramdisk) notes
- The ramdisk directory table is placed at 1MiB offset to avoid overwriting file data.
- The on-disk format used in development is simple and evolving; `mkrnafs` will create/initialize images used by QEMU.
- To add files to ramdisk during image creation, update the directory table entries with file offsets and sizes.

Run/Loader design (phase 1)
- Phase 1 uses a simple RSL flat-binary header with fields: magic, entry_offset, image_size.
- Loader lives in `programs/shell_advanced.c` (userspace). It should:
  - Read file via RNAFS API
  - Validate header
  - `alloc()` memory (syscall) for image
  - Copy image to allocated region
  - Prepare minimal argv/argc on stack
  - Call entry function pointer
  - Rely on `exit()` syscall to return control to shell loader, which should free memory and resume
- Caveat: no process isolation yet — bad programs can crash the system. Test carefully.

mkrnafs (host-side image tool)
- Implement as either a small C program in `tools/` or as a shell script helper in `setup.sh`.
- Default image size: 16MB (configurable). Directory table at 1MiB.
- Use fixed-size directory entries (name 64 bytes, offset, size, flags).

Coding standards and conventions
- Keep changes small and focused; follow existing file styles.
- Use `uint32`/`uint64` types from `include/types.h` for clarity.
- Avoid dynamic allocation in boot/UEFI code — prefer static or controlled heap allocation.
- Prefer explicit error-handling and null checks for all UEFI and driver calls.

Testing and debugging tips
- Use QEMU's `-serial stdio` and `-display none` for headless tests and easy logging.
- For kernel crashes, enable serial output early in `kernel/entry.c`.
- To reproduce builds cleanly, run `make clean && make`.

Adding programs
- Create programs under `programs/` and add compile targets in the Makefile if needed.
- For quick tests, build flat-binary using a small host-side tool that writes the RSL header then raw `.o` or `.bin` payload.

Contributing
- Branch from `main` or the active working branch and open PRs with focused changes.
- Include build and run instructions in PR description; add tests when possible.

Next steps (ideas to implement)
- Implement full userspace loader and sample `hello` program.
- Create `tools/mkrnafs` to produce images and helper scripts to inject files.
- Add `docs/DEVELOPER_GUIDE.md` (this file) to repo and update via PR.

Contact / notes
- This project is evolving. Keep design discussions in PRs and issues to maintain traceable decisions.
- When in doubt, prefer userspace-first changes to preserve boot stability.


-- End of DEVELOPER_GUIDE.md
