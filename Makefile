# OSx2 (RTECH dos) Root Makefile

CC = gcc
LD = ld
OBJCOPY = objcopy

# Paths for EFI build (standard on Debian/Ubuntu)
EFI_LIB = /usr/lib
EFI_LDS = /usr/lib/elf_x86_64_efi.lds
EFI_CRT0 = /usr/lib/crt0-efi-x86_64.o

# Compilation Flags
# Using -DEFI_FUNCTION_WRAPPER if using uefi_call_wrapper (not strictly needed with ms_abi but good for compatibility)
CFLAGS = -Iinclude -fno-stack-protector -fpic \
         -fshort-wchar -mno-red-zone -Wall -fno-builtin -m64

# Linker Flags for EFI
LDFLAGS_EFI = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
              -Bsymbolic -L $(EFI_LIB) $(EFI_CRT0)

# Linker Flags for raw binary (Shell)
LDFLAGS_BIN = -nostdlib -T boot/linker.ld --oformat binary

LIBS = -lefi -lgnuefi

# Kernel Source Files
KERNEL_SRCS = kernel/entry.c \
              kernel/main.c \
              services/io/console.c \
              services/io/input.c \
              services/io/font_data.c \
              services/mem/memory.c \
              services/io/disk.c \
              services/io/diskman.c \
              services/fs/fs.c \
              services/fs/fat.c \
              services/fs/rnafs.c \
              services/core/event.c \
              services/system/power.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)

# Shell Source Files
# libsystem.o must be first for entry point at 0x0
SHELL_SRCS = programs/libsystem.c programs/shell.c
SHELL_OBJS = $(SHELL_SRCS:.c=.o)

# Header files for dependency tracking
HEADERS = $(shell find include kernel services -name "*.h")

# Default Target
all: boot/EFI/BOOT/BOOTX64.EFI boot/shell.bin

# Kernel Build
boot/EFI/BOOT/BOOTX64.EFI: kernel.so
	@mkdir -p boot/EFI/BOOT
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
	           -j .dynsym  -j .rel -j .rela -j .reloc \
	           --target=efi-app-x86_64 kernel.so boot/EFI/BOOT/BOOTX64.EFI

kernel.so: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS_EFI) $(KERNEL_OBJS) -o kernel.so $(LIBS)

# Shell Build
boot/shell.bin: $(SHELL_OBJS)
	$(LD) $(LDFLAGS_BIN) $(SHELL_OBJS) -o boot/shell.bin

# Generic Rule for Object Files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# ISO Image Build
iso: all
	@mkdir -p iso_root/EFI/BOOT
	@cp boot/EFI/BOOT/BOOTX64.EFI iso_root/EFI/BOOT/
	@cp boot/shell.bin iso_root/
	xorriso -as mkisofs -R -f -e EFI/BOOT/BOOTX64.EFI -no-emul-boot \
	        -o osx2.iso iso_root
	@rm -rf iso_root
	@echo "OSx2 ISO created: osx2.iso"

# QEMU Run
run: all
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=fat:rw:boot -net none

# Cleanup
clean:
	@find . -name "*.o" -delete
	@rm -f kernel.so boot/shell.bin osx2.iso
	@rm -rf boot/EFI
	@echo "Cleaned up build artifacts."

.PHONY: all clean run iso
