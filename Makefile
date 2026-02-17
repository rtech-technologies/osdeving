# OSx2 (RTECH dos) Root Makefile

CC = gcc
LD = ld
OBJCOPY = objcopy

# Compilation Flags
CFLAGS = -Iinclude -fno-stack-protector -fpic \
         -fshort-wchar -mno-red-zone -Wall -fno-builtin -m64

LDFLAGS = -nostdlib -znocombreloc -shared -Bsymbolic

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

# Program Source Files
SHELL_SRCS = programs/shell.c programs/libsystem.c
SHELL_OBJS = $(SHELL_SRCS:.c=.o)

# Targets
all: boot/EFI/BOOT/BOOTX64.EFI boot/shell.bin

# Kernel Build
boot/EFI/BOOT/BOOTX64.EFI: kernel/kernel.so
	mkdir -p boot/EFI/BOOT
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
	           -j .dynsym  -j .rel -j .rela -j .reloc \
	           --target=efi-app-x86_64 kernel/kernel.so boot/EFI/BOOT/BOOTX64.EFI

kernel/kernel.so: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o kernel/kernel.so

# Shell Build
boot/shell.bin: $(SHELL_OBJS)
	$(LD) -nostdlib -Ttext 0x0 --oformat binary programs/libsystem.o programs/shell.o -o boot/shell.bin

# Generic Rule for Object Files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# QEMU Run
run: all
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=fat:rw:boot -net none

# Cleanup
clean:
	find . -name "*.o" -delete
	rm -f kernel/kernel.so boot/shell.bin
	rm -rf boot/EFI
