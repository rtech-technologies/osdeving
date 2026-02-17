# OSx2 (RTECH dos) Makefile

CC = gcc
LD = ld
OBJCOPY = objcopy

# Paths
ROOT = ..
BOOT_DIR = $(ROOT)/boot
EFI_DIR = $(BOOT_DIR)/EFI/BOOT

# Paths for EFI build (standard on Debian/Ubuntu)
EFI_LIB = /usr/lib
EFI_LDS = /usr/lib/elf_x86_64_efi.lds
EFI_CRT0 = /usr/lib/crt0-efi-x86_64.o

# Compilation Flags
CFLAGS = -I$(ROOT)/include -fno-stack-protector -fpic \
         -fshort-wchar -mno-red-zone -Wall -fno-builtin -m64 \
         -DEFI_FUNCTION_WRAPPER

# Linker Flags for EFI Shared Object
LDFLAGS = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
          -Bsymbolic -L $(EFI_LIB) $(EFI_CRT0)

LIBS = -lefi -lgnuefi

# Kernel Source Files
KERNEL_SRCS = $(ROOT)/kernel/entry.c \
              $(ROOT)/kernel/main.c \
              $(ROOT)/services/io/console.c \
              $(ROOT)/services/io/input.c \
              $(ROOT)/services/io/font_data.c \
              $(ROOT)/services/mem/memory.c \
              $(ROOT)/services/io/disk.c \
              $(ROOT)/services/io/diskman.c \
              $(ROOT)/services/fs/fs.c \
              $(ROOT)/services/fs/fat.c \
              $(ROOT)/services/fs/rnafs.c \
              $(ROOT)/services/core/event.c \
              $(ROOT)/services/system/power.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)

# Program Source Files
SHELL_SRCS = $(ROOT)/programs/libsystem.c $(ROOT)/programs/shell.c
SHELL_OBJS = $(SHELL_SRCS:.c=.o)

# Header files for dependency tracking
HEADERS = $(shell find $(ROOT)/include $(ROOT)/kernel $(ROOT)/services -name "*.h")

# Targets
all: $(EFI_DIR)/BOOTX64.EFI $(BOOT_DIR)/shell.bin

# Kernel Build
$(EFI_DIR)/BOOTX64.EFI: kernel.so
	@mkdir -p $(EFI_DIR)
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
	           -j .dynsym  -j .rel -j .rela -j .reloc \
	           --target=efi-app-x86_64 kernel.so $(EFI_DIR)/BOOTX64.EFI

kernel.so: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o kernel.so $(LIBS)

# Shell Build
$(BOOT_DIR)/shell.bin: $(SHELL_OBJS)
	# Link libsystem.o first for the entry point
	$(LD) -nostdlib -T $(ROOT)/boot/linker.ld --oformat binary $(ROOT)/programs/libsystem.o $(ROOT)/programs/shell.o -o $(BOOT_DIR)/shell.bin

# Generic Rule for Object Files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# QEMU Run
run: all
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=fat:rw:$(BOOT_DIR) -net none

# Cleanup
clean:
	@find $(ROOT) -name "*.o" -delete
	@rm -f kernel.so $(BOOT_DIR)/shell.bin
	@rm -f $(ROOT)/kernel/kernel.so
	@rm -rf $(ROOT)/boot/EFI
	@echo "Cleaned up build artifacts."

# Setup target
setup:
	@test -f $(EFI_CRT0) || (echo "Error: $(EFI_CRT0) not found"; exit 1)
	@test -f $(EFI_LDS) || (echo "Error: $(EFI_LDS) not found"; exit 1)
	@echo "Dependencies verified."

.PHONY: all clean run setup
