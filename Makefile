# OSx2 (RTECH dos) Root Makefile
# LFS-style: make menuconfig -> make -> make run

# Configuration (sourced from .config, optional)
-include .config

CC = gcc
LD = ld
OBJCOPY = objcopy

# Paths
BOOT_DIR = boot
EFI_DIR = $(BOOT_DIR)/EFI/BOOT

# EFI paths (Ubuntu/Debian)
EFI_LIB = /usr/lib
EFI_LDS = /usr/lib/elf_x86_64_efi.lds
EFI_CRT0 = /usr/lib/crt0-efi-x86_64.o

# Compilation flags
CFLAGS = -Iinclude -fno-stack-protector -fpic -fshort-wchar -mno-red-zone \
         -Wall -fno-builtin -m64 -DEFI_FUNCTION_WRAPPER

LDFLAGS_EFI = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic \
              -L $(EFI_LIB) $(EFI_CRT0)

LDFLAGS_BIN = -nostdlib -T $(BOOT_DIR)/linker.ld --oformat binary

LIBS = -lefi -lgnuefi

# Source files
KERNEL_SRCS = kernel/entry.c kernel/main.c services/console.c \
              services/font_data.c services/input.c services/memory.c \
              services/fs.c services/rnafs.c services/core/event.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)

SHELL_SRCS = programs/libsystem.c programs/shell_advanced.c
SHELL_OBJS = $(SHELL_SRCS:.c=.o)

HEADERS = $(shell find include kernel services -name "*.h")

# === DEFAULT TARGET (must be first) ===

# Build kernel and shell
.PHONY: all
all: $(EFI_DIR)/BOOTX64.EFI $(BOOT_DIR)/shell.bin
	@echo ""
	@echo "✓ OSx2 Kernel & Shell Built"
	@echo ""
	@echo "Next steps:"
	@echo "  make disk   - Create FAT disk image"
	@echo "  make iso    - Create ISO image"
	@echo "  make run    - Test in QEMU"

# Configuration menu
.PHONY: menuconfig
menuconfig:
	@echo "OSx2 Configuration"
	@echo ""
	@echo "Edit .config file to configure build options:"
	@echo "  BUILD_QEMU=1 - Build for QEMU"
	@echo "  BUILD_IMG=1  - Build FAT disk image"
	@echo "  BUILD_ISO=1  - Build ISO image"
	@echo ""
	@echo "Example: BUILD_QEMU=1 make"

# Setup configuration (look for existing or create new)
.PHONY: setup
setup:
	bash setup.sh

# Alias for setup
.PHONY: set
set: setup

# Kernel EFI binary
$(EFI_DIR)/BOOTX64.EFI: kernel.so
	@mkdir -p $(EFI_DIR)
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym \
	           -j .rel -j .rela -j .reloc -j .rodata* \
	           --target=efi-app-x86_64 kernel.so $(EFI_DIR)/BOOTX64.EFI

kernel.so: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS_EFI) $(KERNEL_OBJS) -o kernel.so $(LIBS)

# Shell binary
$(BOOT_DIR)/shell.bin: $(SHELL_OBJS)
	$(LD) $(LDFLAGS_BIN) $(SHELL_OBJS) -o $(BOOT_DIR)/shell.bin

# Object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Disk image
.PHONY: disk
disk: $(BOOT_DIR)/shell.bin
	@echo "Creating disk.img..."
	dd if=/dev/zero of=disk.img bs=1M count=64 2>/dev/null
	mformat -i disk.img -F ::
	mmd -i disk.img ::/EFI ::/EFI/BOOT
	mcopy -i disk.img $(EFI_DIR)/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i disk.img $(BOOT_DIR)/shell.bin ::/
	@echo "✓ disk.img created"

# ISO image
.PHONY: iso
iso: $(BOOT_DIR)/shell.bin
	@echo "Creating osx2.iso..."
	@mkdir -p iso_root/EFI/BOOT
	@cp $(EFI_DIR)/BOOTX64.EFI iso_root/EFI/BOOT/
	@cp $(BOOT_DIR)/shell.bin iso_root/
	xorriso -as mkisofs -R -f -e EFI/BOOT/BOOTX64.EFI -no-emul-boot \
	        -o osx2.iso iso_root 2>/dev/null
	@rm -rf iso_root
	@echo "✓ osx2.iso created"

# QEMU run
.PHONY: run
run: $(BOOT_DIR)/shell.bin
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=fat:rw:boot -net none

# Clean
.PHONY: clean
clean:
	@find . -name "*.o" -delete
	@rm -f kernel.so $(BOOT_DIR)/shell.bin disk.img osx2.iso
	@rm -rf $(BOOT_DIR)/EFI
	@echo "✓ Cleaned"

.PHONY: all clean disk iso run menuconfig setup set
