# OSx2 (RTECH dos) Makefile
# -------------------------

# 1. Architecture & Toolchain
ARCH            = x86_64
EFIINC          = /usr/include/efi
EFIINCS         = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIB             = /usr/lib
EFILIB          = /usr/lib
EFI_CRT_OBJS    = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFILIB)/elf_$(ARCH)_efi.lds

# 2. Compiler Flags
CFLAGS          = $(EFIINCS) -fpic -fshort-wchar -mno-red-zone -Wall \
		  -DEFI_FUNCTION_WRAPPER -fno-builtin -ffreestanding \
		  -Iinclude \
		  -Ikernel/libs/console \
		  -Ikernel/libs/memory \
		  -Ikernel/libs/disk \
		  -Ikernel/libs/fs \
		  -Ikernel/libs/event \
		  -Ikernel/libs/init \
		  -Ikernel/libs/input \
		  -Ikernel/libs/xhci \
		  -Ikernel/libs/pci \
		  -Ikernel/libs/devman \
		  -Ikernel/libs/usb_keyboard \
		  -Ikernel/libs/power \
		  -Ikernel/libs/stup \
		  -Ikernel/unice64

LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
		  -Bsymbolic -L $(EFILIB) -L $(LIB) $(EFI_CRT_OBJS)

# 3. Load Configuration
-include .config
CONFIG_MEMORY_MB ?= 512

# 4. Source Definitions (Sectioned)

# Core Kernel
KERNEL_SRCS = kernel/unice64/main.c \
              kernel/unice64/core.c \
              kernel/libs/init/init.c \
              kernel/libs/event/event.c \
              kernel/libs/stup/stup.c

# Core Services
KERNEL_SRCS += kernel/libs/console/console.c \
               kernel/libs/console/font_data.c \
               kernel/libs/memory/memory.c \
               kernel/libs/input/input_map.c

# Storage Subsystem
KERNEL_SRCS += kernel/libs/disk/disk.c \
               kernel/libs/disk/diskman.c \
               kernel/libs/fs/fs.c

# Hardware Support (Conditional)
ifeq ($(CONFIG_PCI_ENUM),y)
KERNEL_SRCS += kernel/libs/pci/pci.c
endif

ifeq ($(CONFIG_USB_SUPPORT),y)
KERNEL_SRCS += kernel/libs/xhci/xhci.c \
               kernel/libs/usb_keyboard/usb_keyboard.c
endif

ifeq ($(CONFIG_RNAFS),y)
KERNEL_SRCS += kernel/libs/fs/rnafs.c
endif

ifeq ($(CONFIG_FAT16),y)
KERNEL_SRCS += kernel/libs/fs/fat.c
endif

ifeq ($(CONFIG_POWER_SERVICES),y)
KERNEL_SRCS += kernel/libs/power/power.c
endif

# Diagnostics
KERNEL_SRCS += kernel/libs/devman/devman.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)

# 5. Build Artifacts
KERNEL_EFI = BOOTX64.EFI
OVMF_FD = /usr/share/ovmf/OVMF.fd
BOOT_IMG = boot.img
BOOT_ISO = boot.iso

# 6. QEMU Run Configuration
QEMU_BASE_FLAGS = -m $(CONFIG_MEMORY_MB)M -net none -machine pc
QEMU_DEVICES = -device qemu-xhci -device usb-kbd -device usb-mouse -device usb-tablet

# 7. Targets
.PHONY: all clean run run-serial setup compile make vga_test update menuconfig

all: include/config.h $(KERNEL_EFI) shell.bin $(BOOT_IMG) $(BOOT_ISO)

make: all
compile: all

setup:
	sudo apt-get update && sudo apt-get install -y gnu-efi build-essential qemu-system-x86 ovmf dosfstools mtools xorriso
	mkdir -p boot kernel/libs kernel/unice64 include programs scripts
	@if [ ! -f boot/linker.ld ]; then \
		echo "SECTIONS { . = 0x0; .text : { *(.text) } .rodata : { *(.rodata) } .data : { *(.data) } .bss : { *(.bss) } }" > boot/linker.ld; \
	fi

menuconfig:
	python3 scripts/menuconfig.py

include/config.h:
	python3 scripts/menuconfig.py --default

$(KERNEL_EFI): kernel.so
	objcopy -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym  -j .rel -j .rela -j .reloc \
		-j .rodata -j .rodata* \
		--target=efi-app-$(ARCH) $^ $@

kernel.so: $(KERNEL_OBJS)
	ld $(LDFLAGS) $(KERNEL_OBJS) -o $@ -lefi -lgnuefi

# Shell compilation
shell.bin: programs/shell.c programs/stub.c programs/libsystem.c boot/linker.ld
	cc $(CFLAGS) -Iinclude -c programs/shell.c -o programs/shell.o
	cc $(CFLAGS) -Iinclude -c programs/stub.c -o programs/stub.o
	cc $(CFLAGS) -Iinclude -c programs/libsystem.c -o programs/libsystem.o
	ld -nostdlib -T boot/linker.ld --entry=_start programs/stub.o programs/shell.o programs/libsystem.o -o shell.elf
	objcopy -O binary shell.elf shell.bin

$(BOOT_IMG): $(KERNEL_EFI) shell.bin
	dd if=/dev/zero of=$(BOOT_IMG) bs=1M count=64
	mkfs.fat -F 32 $(BOOT_IMG)
	mmd -i $(BOOT_IMG) ::/EFI
	mmd -i $(BOOT_IMG) ::/EFI/BOOT
	mcopy -i $(BOOT_IMG) $(KERNEL_EFI) ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i $(BOOT_IMG) shell.bin ::/shell.bin

$(BOOT_ISO): $(BOOT_IMG)
	mkdir -p iso_root/EFI/BOOT
	cp $(KERNEL_EFI) iso_root/EFI/BOOT/BOOTX64.EFI
	cp shell.bin iso_root/shell.bin
	cp $(BOOT_IMG) iso_root/efiboot.img
	xorriso -as mkisofs -R -f -e efiboot.img -no-emul-boot -o $(BOOT_ISO) iso_root

run: all
	qemu-system-x86_64 \
		-bios $(OVMF_FD) \
		-drive format=raw,file=$(BOOT_IMG) \
		$(QEMU_BASE_FLAGS) -serial stdio -display sdl \
		$(QEMU_DEVICES)

run-serial: all
	qemu-system-x86_64 \
		-nographic \
		-bios $(OVMF_FD) \
		-drive format=raw,file=$(BOOT_IMG) \
		$(QEMU_BASE_FLAGS) \
		$(QEMU_DEVICES)

vga_test:
	@echo "VGA testing placeholder"

clean:
	rm -f kernel.so $(KERNEL_EFI) \
	      kernel/unice64/*.o \
	      kernel/libs/console/*.o \
	      kernel/libs/memory/*.o \
	      kernel/libs/disk/*.o \
	      kernel/libs/disk/diskman.o \
	      kernel/libs/fs/*.o \
	      kernel/libs/event/*.o \
	      kernel/libs/init/*.o \
	      kernel/libs/input/*.o \
	      kernel/libs/xhci/*.o \
	      kernel/libs/pci/*.o \
	      kernel/libs/devman/*.o \
	      kernel/libs/usb_keyboard/*.o \
	      kernel/libs/power/*.o \
	      kernel/libs/stup/*.o \
	      programs/*.o shell.elf shell.bin
	rm -f $(BOOT_IMG) $(BOOT_ISO)
	rm -rf disk iso_root

update: clean
	mkdir -p ~/Downloads
	cp -r . ~/Downloads/osx2_repo
	rm -rf $(CURDIR)
