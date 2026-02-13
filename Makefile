ARCH            = x86_64
EFIINC          = /usr/include/efi
EFIINCS         = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIB             = /usr/lib
EFILIB          = /usr/lib
EFI_CRT_OBJS    = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFILIB)/elf_$(ARCH)_efi.lds

# Removed -DGNU_EFI_USE_MS_ABI to stick with gnu-efi default behavior
CFLAGS          = $(EFIINCS) -fpic -fshort-wchar -mno-red-zone -Wall \
		  -DEFI_FUNCTION_WRAPPER -fno-builtin -ffreestanding

LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
		  -Bsymbolic -L $(EFILIB) -L $(LIB) $(EFI_CRT_OBJS)

KERNEL_SRCS = kernel/main.c services/console.c services/memory.c services/event.c services/disk.c services/fs.c
KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)

KERNEL_EFI = BOOTX64.EFI
OVMF_FD = /usr/share/ovmf/OVMF.fd
QEMU_DISPLAY = -nographic

.PHONY: all clean run setup

all: $(KERNEL_EFI) shell.bin boot.img

setup:
	sudo apt-get update && sudo apt-get install -y gnu-efi build-essential qemu-system-x86 ovmf dosfstools mtools
	mkdir -p boot build kernel services include programs
	@if [ ! -f programs/linker.ld ]; then \
		echo "SECTIONS { . = 0x0; .text : { *(.text) } .rodata : { *(.rodata) } .data : { *(.data) } .bss : { *(.bss) } }" > programs/linker.ld; \
	fi
	cp programs/linker.ld boot/linker.ld

$(KERNEL_EFI): kernel.so
	objcopy -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym  -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) $^ $@

kernel.so: $(KERNEL_OBJS)
	ld $(LDFLAGS) $(KERNEL_OBJS) -o $@ -lefi -lgnuefi

# Shell compilation
shell.bin: programs/shell.c programs/stub.c services/console.c services/memory.c services/event.c
	cc $(CFLAGS) -Iinclude -c programs/shell.c -o programs/shell.o
	cc $(CFLAGS) -Iinclude -c programs/stub.c -o programs/stub.o
	# Note: we are linking service objects into the shell for direct calls
	ld -nostdlib -T programs/linker.ld --entry=_start programs/stub.o programs/shell.o services/console.o services/memory.o services/event.o -o shell.elf
	objcopy -O binary shell.elf shell.bin

boot.img: $(KERNEL_EFI) shell.bin
	dd if=/dev/zero of=boot.img bs=1M count=64
	mkfs.fat -F 32 boot.img
	mmd -i boot.img ::/EFI
	mmd -i boot.img ::/EFI/BOOT
	mcopy -i boot.img $(KERNEL_EFI) ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i boot.img shell.bin ::/shell.bin

run: all
	qemu-system-x86_64 $(QEMU_DISPLAY) \
		-bios $(OVMF_FD) \
		-drive format=raw,file=boot.img \
		-m 512M \
		-net none

clean:
	rm -f kernel.so $(KERNEL_EFI) kernel/main.o services/*.o programs/*.o shell.elf shell.bin
	rm -f boot.img
	rm -rf disk
