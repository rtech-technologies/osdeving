# OSx2 (RTECH dos) Root Makefile

CC = gcc
LD = ld
OBJCOPY = objcopy

# Load Configuration if exists
-include .config

# Paths
BOOT_DIR = boot
EFI_DIR = $(BOOT_DIR)/EFI/BOOT
KERNEL_DIR = kernel/unice64
LIBS_DIR = kernel/libs

# Paths for EFI build
EFI_LIB = /usr/lib
EFI_LDS = /usr/lib/elf_x86_64_efi.lds
EFI_CRT0 = /usr/lib/crt0-efi-x86_64.o

# Compilation Flags
CFLAGS = -Iinclude -Ikernel/libs -Ikernel/libs/core -fno-stack-protector -fpic \
         -fshort-wchar -mno-red-zone -Wall -fno-builtin -m64 \
         -DEFI_FUNCTION_WRAPPER

# Linker Flags
LDFLAGS_EFI = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
              -Bsymbolic -L $(EFI_LIB) $(EFI_CRT0)
LDFLAGS_BIN = -nostdlib -T $(BOOT_DIR)/linker.ld --oformat binary

LIBS = -lefi -lgnuefi

# Kernel Source Files
KERNEL_SRCS = $(KERNEL_DIR)/entry.c \
              $(KERNEL_DIR)/main.c \
              $(KERNEL_DIR)/gdt.c \
              $(LIBS_DIR)/console.c \
              $(LIBS_DIR)/font_data.c \
              $(LIBS_DIR)/input.c \
              $(LIBS_DIR)/disk.c \
              $(LIBS_DIR)/diskman.c \
              $(LIBS_DIR)/memory.c \
              $(LIBS_DIR)/fs.c \
              $(LIBS_DIR)/rnafs.c \
              $(LIBS_DIR)/loader.c \
              $(LIBS_DIR)/core/event.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)

# Shell Source Files
SHELL_SRCS = programs/libsystem.c programs/shell.c
SHELL_OBJS = $(SHELL_SRCS:.c=.o)

HEADERS = $(shell find include kernel -name "*.h")

# Default Target
all: prepare $(EFI_DIR)/BOOTX64.EFI $(BOOT_DIR)/shell.bin

prepare:
	@if [ ! -f .config ]; then \
		echo "No .config found, using defaults..."; \
		echo "CONFIG_DEBUG_LOGS=y" > .config; \
		echo "CONFIG_HEAP_SIZE_MB=4" >> .config; \
		echo "CONFIG_SCROLL_SPEED=10" >> .config; \
		echo "CONFIG_EMERALD_MODE=y" >> .config; \
		echo "CONFIG_LOAD_SHELL=y" >> .config; \
		python3 scripts/menuconfig.py --save; \
	fi

menuconfig:
	python3 scripts/menuconfig.py

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

# Force explicit rules to avoid builtin weirdness
kernel/unice64/%.o: kernel/unice64/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

kernel/libs/%.o: kernel/libs/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

kernel/libs/core/%.o: kernel/libs/core/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

programs/%.o: programs/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# OS Disk Image Build (GPT Simulation)
disk: all
	dd if=/dev/zero of=disk.img bs=1M count=64
	@# Write GPT Signature at LBA 1 (Offset 512)
	printf "EFI PART" | dd of=disk.img bs=1 seek=512 conv=notrunc
	@echo "OSx2 GPT Disk Image created (64MB raw). GPT Header signature injected."

# QEMU Run
run: all
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=fat:rw:boot -net none

# Cleanup
clean:
	@find . -name "*.o" -delete
	@rm -f kernel.so $(BOOT_DIR)/shell.bin osx2.iso disk.img .config include/config.h
	@rm -rf $(BOOT_DIR)/EFI
	@echo "Cleaned up build artifacts."

.PHONY: all clean run menuconfig prepare disk
