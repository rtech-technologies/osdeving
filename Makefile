# OSx2 (RTECH dos) Root Makefile

CC = gcc
LD = ld
OBJCOPY = objcopy

# Paths
BOOT_DIR = boot
EFI_DIR = $(BOOT_DIR)/EFI/BOOT

# Paths for EFI build (standard on Debian/Ubuntu)
EFI_LIB = /usr/lib
EFI_LDS = /usr/lib/elf_x86_64_efi.lds
EFI_CRT0 = /usr/lib/crt0-efi-x86_64.o

# Compilation Flags
CFLAGS = -Iinclude -fno-stack-protector -fpic \
         -fshort-wchar -mno-red-zone -Wall -fno-builtin -m64 \
         -DEFI_FUNCTION_WRAPPER

# Linker Flags for EFI Shared Object
LDFLAGS_EFI = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
              -Bsymbolic -L $(EFI_LIB) $(EFI_CRT0)

# Linker Flags for raw binary (Shell)
LDFLAGS_BIN = -nostdlib -T $(BOOT_DIR)/linker.ld --oformat binary

LIBS = -lefi -lgnuefi

# Kernel Source Files
KERNEL_SRCS = kernel/entry.c \
              kernel/main.c \
              services/console.c \
              services/font_data.c \
              services/input.c \
              services/memory.c \
              services/fs.c \
              services/rnafs.c \
              services/core/event.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)

# Shell Source Files
# libsystem.o must be first for entry point at 0x0
SHELL_SRCS = programs/libsystem.c programs/shell.c
SHELL_OBJS = $(SHELL_SRCS:.c=.o)

# Header files for dependency tracking
HEADERS = $(shell find include kernel services -name "*.h")

# Default Target
all: $(EFI_DIR)/BOOTX64.EFI $(BOOT_DIR)/shell.bin

# Kernel Build
$(EFI_DIR)/BOOTX64.EFI: kernel.so
	@mkdir -p $(EFI_DIR)
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
	           -j .dynsym  -j .rel -j .rela -j .reloc \
	           -j .rodata* --target=efi-app-x86_64 kernel.so $(EFI_DIR)/BOOTX64.EFI

kernel.so: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS_EFI) $(KERNEL_OBJS) -o kernel.so $(LIBS)

# Shell Build
$(BOOT_DIR)/shell.bin: $(SHELL_OBJS)
	$(LD) $(LDFLAGS_BIN) $(SHELL_OBJS) -o $(BOOT_DIR)/shell.bin

# Generic Rule for Object Files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# ISO Image Build
iso: all
	@mkdir -p iso_root/EFI/BOOT
	@cp $(EFI_DIR)/BOOTX64.EFI iso_root/EFI/BOOT/
	@cp $(BOOT_DIR)/shell.bin iso_root/
	xorriso -as mkisofs -R -f -e EFI/BOOT/BOOTX64.EFI -no-emul-boot \
	        -o osx2.iso iso_root
	@rm -rf iso_root
	@echo "OSx2 ISO created: osx2.iso"

# Disk Image Build
disk: all
	dd if=/dev/zero of=disk.img bs=1M count=64
	mformat -i disk.img -F ::
	mmd -i disk.img ::/EFI
	mmd -i disk.img ::/EFI/BOOT
	mcopy -i disk.img $(EFI_DIR)/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i disk.img $(BOOT_DIR)/shell.bin ::/
	@echo "OSx2 Disk Image created: disk.img"

# QEMU Run
run: all
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=fat:rw:boot -net none

# Cleanup
clean:
	@find . -name "*.o" -delete
	@rm -f kernel.so $(BOOT_DIR)/shell.bin osx2.iso disk.img
	@rm -rf $(BOOT_DIR)/EFI
	@echo "Cleaned up build artifacts."

.PHONY: all clean run iso disk
