# OSx2 (RTECH dos) Root Makefile

CC = gcc
LD = ld
OBJCOPY = objcopy

# Load Configuration
-include .config

# Paths
BOOT_DIR = boot
EFI_DIR = $(BOOT_DIR)/EFI/BOOT
LOADER_DIR = loader
KERNEL_DIR = kernel/unice64
LIBS_DIR = kernel/libs

# GNU-EFI Paths (Adjust if needed)
EFI_LIB = /usr/lib
EFI_LDS = /usr/lib/elf_x86_64_efi.lds
EFI_CRT0 = /usr/lib/crt0-efi-x86_64.o

# Flags
CFLAGS_COMMON = -Iinclude -fno-stack-protector -mno-red-zone -Wall -fno-builtin -m64 -O0
CFLAGS_EFI = $(CFLAGS_COMMON) -fpic -fshort-wchar -DEFI_FUNCTION_WRAPPER
CFLAGS_KERNEL = $(CFLAGS_COMMON) -I$(LIBS_DIR) -I$(LIBS_DIR)/core -I$(KERNEL_DIR) -ffreestanding

LDFLAGS_EFI = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(EFI_LIB) $(EFI_CRT0)
LDFLAGS_KERNEL = -nostdlib -T $(BOOT_DIR)/linker.ld --oformat binary
LDFLAGS_SHELL = -nostdlib -T $(BOOT_DIR)/linker.ld --oformat binary

LIBS_EFI = -lefi -lgnuefi

# Source Files
LOADER_SRCS = $(LOADER_DIR)/entry.c
LOADER_OBJS = $(LOADER_SRCS:.c=.o)

KERNEL_SRCS = $(KERNEL_DIR)/main.c \
              $(KERNEL_DIR)/gdt.c \
              $(LIBS_DIR)/kutils.c \
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

SHELL_SRCS = programs/libsystem.c programs/shell.c
SHELL_OBJS = $(SHELL_SRCS:.c=.o)

HEADERS = $(shell find include kernel loader -name "*.h")

# Default Target
all: prepare $(EFI_DIR)/BOOTX64.EFI $(BOOT_DIR)/kernel.bin $(BOOT_DIR)/shell.bin

prepare:
	@if [ ! -f .config ]; then \
		echo "No .config found, using defaults..."; \
		python3 scripts/menuconfig.py --save; \
	fi

# Stage 1: Loader (EFI)
$(EFI_DIR)/BOOTX64.EFI: loader.so
	@mkdir -p $(EFI_DIR)
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
	           -j .dynsym  -j .rel -j .rela -j .reloc \
	           -j .rodata* --target=efi-app-x86_64 loader.so $(EFI_DIR)/BOOTX64.EFI

loader.so: $(LOADER_OBJS)
	$(LD) $(LDFLAGS_EFI) $(LOADER_OBJS) -o loader.so $(LIBS_EFI)

$(LOADER_DIR)/%.o: $(LOADER_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS_EFI) -c $< -o $@

# Stage 2: Kernel (Raw Binary)
$(BOOT_DIR)/kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS_KERNEL) $(KERNEL_OBJS) -o $(BOOT_DIR)/kernel.bin

$(KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS_KERNEL) -c $< -o $@

$(LIBS_DIR)/%.o: $(LIBS_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS_KERNEL) -c $< -o $@

$(LIBS_DIR)/core/%.o: $(LIBS_DIR)/core/%.c $(HEADERS)
	$(CC) $(CFLAGS_KERNEL) -c $< -o $@

# Programs (Raw Binary)
$(BOOT_DIR)/shell.bin: $(SHELL_OBJS)
	$(LD) $(LDFLAGS_SHELL) $(SHELL_OBJS) -o $(BOOT_DIR)/shell.bin

programs/%.o: programs/%.c $(HEADERS)
	$(CC) $(CFLAGS_KERNEL) -c $< -o $@

# Disk Image
disk: all
	dd if=/dev/zero of=disk.img bs=1M count=64
	python3 scripts/rnafs_tool.py disk.img format
	python3 scripts/rnafs_tool.py disk.img add $(BOOT_DIR)/shell.bin shell.bin
	@echo "OSx2 Disk Image created. kernel.bin must be manually placed in EFI path for loader to find it."
	@# For simplicity in v1.2, the loader expects kernel.bin in the root of EFI partition
	mcopy -i disk.img $(BOOT_DIR)/kernel.bin ::/kernel.bin

# Cleanup
clean:
	@find . -name "*.o" -delete
	@rm -f loader.so $(BOOT_DIR)/kernel.bin $(BOOT_DIR)/shell.bin disk.img .config include/config.h
	@rm -rf $(BOOT_DIR)/EFI
	@echo "Cleaned up."

.PHONY: all clean disk prepare
