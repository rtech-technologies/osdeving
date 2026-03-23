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

LDFLAGS_EFI = -nostdlib -znocombreloc -z max-page-size=0x1000 -T $(EFI_LDS) -shared -Bsymbolic -L $(EFI_LIB) $(EFI_CRT0)
LDFLAGS_SHELL = -nostdlib -T programs/linker.ld

LIBS_EFI = -lefi -lgnuefi

# Source Files
KERNEL_OBJS = $(KERNEL_DIR)/efi_entry.o \
              $(KERNEL_DIR)/main.o \
              $(KERNEL_DIR)/gdt.o \
              $(KERNEL_DIR)/paging.o \
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
              $(LIBS_DIR)/loader.o \
              $(LIBS_DIR)/core/event.o

SHELL_SRCS = programs/entry.S programs/libsystem.c programs/shell.c
SHELL_OBJS = programs/entry.o programs/libsystem.o programs/shell.o

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
	           -j .rodata* --target=efi-app-x86_64 \
	           --section-alignment 4096 kernel.so $(EFI_DIR)/BOOTX64.EFI

kernel.so: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS_EFI) $(KERNEL_OBJS) -o kernel.so $(LIBS_EFI)

$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS_EFI) -c $< -o $@

$(LIBS_DIR)/%.o: $(LIBS_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS_EFI) -c $< -o $@

$(LIBS_DIR)/core/%.o: $(LIBS_DIR)/core/%.c $(HEADERS)
	$(CC) $(CFLAGS_EFI) -c $< -o $@

# Programs (Pure Flat Binary)
$(BOOT_DIR)/os2.bin: os2.elf
	$(OBJCOPY) -O binary --strip-all os2.elf $(BOOT_DIR)/os2.bin
	@# Pad to 4KB boundary
	@truncate -s %4096 $(BOOT_DIR)/os2.bin

os2.elf: $(SHELL_OBJS)
	$(LD) $(LDFLAGS_SHELL) $(SHELL_OBJS) -o os2.elf

programs/%.o: programs/%.c $(HEADERS)
	$(CC) -Iinclude -ffreestanding -fno-stack-protector -mno-red-zone -Wall -fno-builtin -m64 -nostdlib -static -c $< -o $@

programs/%.o: programs/%.S
	$(CC) -Iinclude -c $< -o $@

# Advanced Tools: Create Bootable UEFI Disk Image (GPT-ESP Edition)
disk: all $(BOOT_DIR)/ramdisk.img
	@echo "Creating bootable UEFI disk image (GPT-ESP)..."
	dd if=/dev/zero of=disk.img bs=1M count=256
	parted disk.img -s mklabel gpt
	# Create one large ESP
	parted disk.img -s mkpart primary fat32 2048s 100%
	parted disk.img -s set 1 esp on

	# Format the partition at 1MB offset
	mformat -i disk.img@@1M -F -v "OSX2" ::
	mmd -i disk.img@@1M ::/EFI
	mmd -i disk.img@@1M ::/EFI/BOOT
	mcopy -i disk.img@@1M $(EFI_DIR)/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i disk.img@@1M $(BOOT_DIR)/os2.bin ::/os2.bin
	mcopy -i disk.img@@1M $(BOOT_DIR)/ramdisk.img ::/ramdisk.img
	echo "FS0:\\EFI\\BOOT\\BOOTX64.EFI" > startup.nsh
	mcopy -i disk.img@@1M startup.nsh ::/startup.nsh
	rm startup.nsh
	@echo "OSx2 UEFI GPT Disk Image Ready (disk.img)."

$(BOOT_DIR)/ramdisk.img:
	@echo "Generating system ramdisk..."
	dd if=/dev/zero of=$(BOOT_DIR)/ramdisk.img bs=1M count=16
	mkfs.vfat -F 32 -n "OSX2_RAM" $(BOOT_DIR)/ramdisk.img

# Create a bootable UEFI ISO (GPT-Hybrid Edition)
iso: all $(BOOT_DIR)/ramdisk.img
	@echo "Creating bootable UEFI ISO image (GPT Hybrid)..."
	mkdir -p iso/EFI/BOOT

	# 1. Create the unified ESP image (includes Loader and Kernel)
	dd if=/dev/zero of=esp.img bs=1M count=128
	mkfs.vfat -F 32 -n "OSX2" esp.img
	mmd -i esp.img ::/EFI
	mmd -i esp.img ::/EFI/BOOT
	mcopy -i esp.img $(EFI_DIR)/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i esp.img $(BOOT_DIR)/os2.bin ::/os2.bin
	mcopy -i esp.img $(BOOT_DIR)/ramdisk.img ::/ramdisk.img
	echo "FS0:\\EFI\\BOOT\\BOOTX64.EFI" > startup.nsh
	mcopy -i esp.img startup.nsh ::/startup.nsh
	rm startup.nsh

	# 2. Create Hybrid ISO
	mv esp.img iso/esp.img
	xorriso -as mkisofs \
		-R -J -V "OSX2_INSTALL" \
		-eltorito-platform efi \
		-e esp.img -no-emul-boot \
		-append_partition 2 0xef iso/esp.img \
		-isohybrid-gpt-basdat \
		-o boot.iso iso/
	@echo "OSx2 Boot ISO Ready (boot.iso)."

run: iso
	qemu-system-x86_64 -machine q35 -bios /usr/share/ovmf/OVMF.fd -cdrom boot.iso -m 256M -serial stdio -net none

debug: iso
	qemu-system-x86_64 -machine q35 -bios /usr/share/ovmf/OVMF.fd \
		-drive file=boot.iso,format=raw \
		-m 2G \
		-no-reboot -no-shutdown \
		-d int,cpu_reset,guest_errors \
		-D qemu.log \
		-serial stdio \
		-monitor vc \
		-s -S

gdb:
	gdb -ex "target remote localhost:1234" \
	    -ex "symbol-file kernel.so" \
	    -ex "set architecture i386:x86-64" \
	    -ex "layout src" \
	    -ex "break kernel_main"

setup:
	sudo apt-get update
	sudo apt-get install -y gnu-efi build-essential qemu-system-x86 ovmf mtools dosfstools xorriso parted gdb

# Cleanup
clean:
	@find . -name "*.o" -delete
	@rm -f kernel.so $(BOOT_DIR)/os2.bin $(BOOT_DIR)/ramdisk.img disk.img boot.iso
	@rm -rf $(BOOT_DIR)/EFI iso
	@echo "Build artifacts removed."

.PHONY: all clean disk iso prepare menuconfig info run
