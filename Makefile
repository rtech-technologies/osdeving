# OSx2 (RTECH dos) Root Makefile - Native UEFI Edition

CC = gcc
LD = ld
OBJCOPY = objcopy

# Load Configuration
-include .config

# Set defaults if not configured
CONFIG_KERNEL_BASE ?= 0x100000
CONFIG_HEAP_BASE   ?= 0x2000000
CONFIG_OPTIMIZATION ?= 0

# Paths
BOOT_DIR = boot
EFI_DIR = $(BOOT_DIR)/EFI/BOOT
KERNEL_DIR = kernel/unice64
LIBS_DIR = kernel/libs

# GNU-EFI Paths
EFI_LIB = /usr/lib
EFI_LDS = /usr/lib/elf_x86_64_efi.lds
EFI_CRT0 = /usr/lib/crt0-efi-x86_64.o

# Flags
# Use EFI flags for the entire kernel now
CFLAGS_EFI = -Iinclude -fno-stack-protector -mno-red-zone -Wall -fno-builtin -m64 -O$(CONFIG_OPTIMIZATION) \
             -fpic -fshort-wchar -DEFI_FUNCTION_WRAPPER -I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol

LDFLAGS_EFI = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(EFI_LIB) $(EFI_CRT0)
LDFLAGS_SHELL = -nostdlib -T programs/linker.ld --oformat binary

LIBS_EFI = -lefi -lgnuefi

# Source Files
KERNEL_OBJS = $(KERNEL_DIR)/efi_entry.o \
              $(KERNEL_DIR)/main.o \
              $(KERNEL_DIR)/gdt.o \
              $(LIBS_DIR)/kutils.o \
              $(LIBS_DIR)/console.o \
              $(LIBS_DIR)/font_data.o \
              $(LIBS_DIR)/input.o \
              $(LIBS_DIR)/connect.o \
              $(LIBS_DIR)/vdisk.o \
              $(LIBS_DIR)/disk.o \
              $(LIBS_DIR)/diskman.o \
              $(LIBS_DIR)/memory.o \
              $(LIBS_DIR)/fs.o \
              $(LIBS_DIR)/rnafs.o \
              $(LIBS_DIR)/loader.o \
              $(LIBS_DIR)/core/event.o

SHELL_SRCS = programs/libsystem.c programs/shell.c
SHELL_OBJS = $(SHELL_SRCS:.c=.o)

HEADERS = $(shell find include kernel -name "*.h")

# Default Target
all: info prepare $(EFI_DIR)/BOOTX64.EFI $(BOOT_DIR)/os2.bin

info:
	@echo "------------------------------------------------"
	@echo " OSx2 / RTECH dos Build System (Native UEFI)"
	@echo " Optimization Level: -O$(CONFIG_OPTIMIZATION)"
	@echo "------------------------------------------------"

prepare:
	@if [ ! -f .config ]; then \
		python3 scripts/menuconfig.py --save; \
	fi

menuconfig:
	@python3 scripts/menuconfig.py

# Unified Kernel (Native EFI App)
$(EFI_DIR)/BOOTX64.EFI: kernel.so
	@mkdir -p $(EFI_DIR)
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
	           -j .dynsym  -j .rel -j .rela -j .reloc \
	           -j .rodata* --target=efi-app-x86_64 kernel.so $(EFI_DIR)/BOOTX64.EFI

kernel.so: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS_EFI) $(KERNEL_OBJS) -o kernel.so $(LIBS_EFI)

$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS_EFI) -c $< -o $@

$(LIBS_DIR)/%.o: $(LIBS_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS_EFI) -c $< -o $@

$(LIBS_DIR)/core/%.o: $(LIBS_DIR)/core/%.c $(HEADERS)
	$(CC) $(CFLAGS_EFI) -c $< -o $@

# Programs (Still raw binary for simplicity in loading)
$(BOOT_DIR)/os2.bin: $(SHELL_OBJS)
	$(LD) $(LDFLAGS_SHELL) $(SHELL_OBJS) -o $(BOOT_DIR)/os2.bin

programs/%.o: programs/%.c $(HEADERS)
	$(CC) -Iinclude -fno-stack-protector -mno-red-zone -Wall -fno-builtin -m64 -ffreestanding -c $< -o $@

# Advanced Tools: Create Bootable UEFI Disk Image
disk: all
	@echo "Creating bootable UEFI disk image..."
	dd if=/dev/zero of=disk.img bs=1M count=64
	parted disk.img -s mklabel gpt
	parted disk.img -s mkpart primary fat32 2048s 100%
	parted disk.img -s set 1 esp on
	mformat -i disk.img@@1M -F -v "OSX2" ::
	mmd -i disk.img@@1M ::/EFI
	mmd -i disk.img@@1M ::/EFI/BOOT
	mcopy -i disk.img@@1M $(EFI_DIR)/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i disk.img@@1M $(BOOT_DIR)/os2.bin ::/os2.bin
	@echo "OSx2 Pro UEFI Disk Image Ready (disk.img)."

# Create a bootable UEFI ISO (Dual-method)
iso: all
	@echo "Creating bootable UEFI ISO image..."
	mkdir -p iso/EFI/BOOT
	cp $(EFI_DIR)/BOOTX64.EFI iso/EFI/BOOT/BOOTX64.EFI
	cp $(BOOT_DIR)/os2.bin iso/os2.bin
	# Create a FAT image for the EFI boot sector
	dd if=/dev/zero of=efiboot.img bs=1K count=8192
	mformat -i efiboot.img -F ::
	mmd -i efiboot.img ::/EFI
	mmd -i efiboot.img ::/EFI/BOOT
	mcopy -i efiboot.img $(EFI_DIR)/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i efiboot.img $(BOOT_DIR)/os2.bin ::/os2.bin
	mmd -i efiboot.img ::/OS2
	mcopy -i efiboot.img $(BOOT_DIR)/os2.bin ::/OS2/os2.bin
	cp efiboot.img iso/efiboot.img
	xorriso -as mkisofs \
		-R -f \
		-e efiboot.img \
		-no-emul-boot \
		-eltorito-platform efi \
		-o boot.iso iso/
	rm efiboot.img
	@echo "OSx2 Boot ISO Ready (boot.iso)."

run: iso
	qemu-system-x86_64 -machine q35 -bios /usr/share/ovmf/OVMF.fd -cdrom boot.iso -m 256M -serial stdio -net none

setup:
	sudo apt-get update
	sudo apt-get install -y gnu-efi build-essential qemu-system-x86 ovmf mtools dosfstools xorriso parted

# Cleanup
clean:
	@find . -name "*.o" -delete
	@rm -f kernel.so $(BOOT_DIR)/os2.bin disk.img boot.iso
	@rm -rf $(BOOT_DIR)/EFI iso
	@echo "Build artifacts removed."

.PHONY: all clean disk iso prepare menuconfig info run
