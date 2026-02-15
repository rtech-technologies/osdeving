ARCH            = x86_64
EFIINC          = /usr/include/efi
EFIINCS         = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIB             = /usr/lib
EFILIB          = /usr/lib
EFI_CRT_OBJS    = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFILIB)/elf_$(ARCH)_efi.lds

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
		  -Ikernel/libs/usb_keyboard \
		  -Ikernel/libs/stup \
		  -Ikernel/unice64

LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
		  -Bsymbolic -L $(EFILIB) -L $(LIB) $(EFI_CRT_OBJS)

KERNEL_SRCS = kernel/unice64/main.c \
              kernel/unice64/core.c \
              kernel/libs/init/init.c \
              kernel/libs/console/console.c \
              kernel/libs/console/font_data.c \
              kernel/libs/memory/memory.c \
              kernel/libs/disk/disk.c \
              kernel/libs/fs/fs.c \
              kernel/libs/event/event.c \
              kernel/libs/input/input_map.c \
              kernel/libs/xhci/xhci.c \
              kernel/libs/usb_keyboard/usb_keyboard.c \
              kernel/libs/stup/stup.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)

KERNEL_EFI = BOOTX64.EFI
OVMF_FD = /usr/share/ovmf/OVMF.fd
BOOT_IMG = boot.img
# QEMU Run Configuration
QEMU_BASE_FLAGS = -m 512M -net none
QEMU_DEVICES = -device qemu-xhci -device usb-kbd -device usb-mouse -device usb-tablet

.PHONY: all clean run run-serial setup compile make vga_test

all: $(KERNEL_EFI) shell.bin $(BOOT_IMG)

make: all

compile: all

setup:
	sudo apt-get update && sudo apt-get install -y gnu-efi build-essential qemu-system-x86 ovmf dosfstools mtools
	mkdir -p boot kernel/libs kernel/unice64 include programs
	@if [ ! -f boot/linker.ld ]; then \
		echo "SECTIONS { . = 0x0; .text : { *(.text) } .rodata : { *(.rodata) } .data : { *(.data) } .bss : { *(.bss) } }" > boot/linker.ld; \
	fi

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
	      kernel/libs/fs/*.o \
	      kernel/libs/event/*.o \
	      kernel/libs/init/*.o \
	      kernel/libs/input/*.o \
	      kernel/libs/xhci/*.o \
      kernel/libs/usb_keyboard/*.o \
	      kernel/libs/stup/*.o \
	      programs/*.o shell.elf shell.bin
	rm -f $(BOOT_IMG)
	rm -rf disk
