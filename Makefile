ARCH            = x86_64
EFIINC          = /usr/include/efi
EFIINCS         = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIB             = /usr/lib
EFILIB          = /usr/lib
EFI_CRT_OBJS    = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFILIB)/elf_$(ARCH)_efi.lds

CFLAGS          = $(EFIINCS) -fno-stack-protector -fpic \
		  -fshort-wchar -mno-red-zone -Wall \
		  -DEFI_FUNCTION_WRAPPER -fno-builtin
ifeq ($(ARCH),x86_64)
  CFLAGS += -DGNU_EFI_USE_MS_ABI
endif

LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
		  -Bsymbolic -L $(EFILIB) -L $(LIB) $(EFI_CRT_OBJS)

KERNEL_SRCS = kernel/main.c services/console.c services/memory.c services/event.c services/disk.c services/fs.c
KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)

BOOT_DIR = boot
KERNEL_EFI = $(BOOT_DIR)/bootx64.efi
OVMF_FD = /usr/share/ovmf/OVMF.fd

.PHONY: all clean run setup

all: $(KERNEL_EFI) shell.bin

setup:
	sudo apt-get update && sudo apt-get install -y gnu-efi build-essential qemu-system-x86 ovmf

$(KERNEL_EFI): kernel.so
	mkdir -p $(BOOT_DIR)
	objcopy -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym  -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) $^ $@

kernel.so: $(KERNEL_OBJS)
	ld $(LDFLAGS) $(KERNEL_OBJS) -o $@ -lefi -lgnuefi

# Shell compilation
shell.bin: programs/shell.c programs/stub.c services/console.c services/memory.c services/event.c
	cc $(CFLAGS) -Iinclude -c programs/shell.c -o programs/shell.o
	cc $(CFLAGS) -Iinclude -c programs/stub.c -o programs/stub.o
	# stub.o must be first to ensure _start is at the beginning of the binary
	ld -nostdlib -T programs/linker.ld --entry=_start programs/stub.o programs/shell.o services/console.o services/memory.o services/event.o -o shell.elf
	objcopy -O binary shell.elf shell.bin

run: all
	mkdir -p disk/EFI/BOOT
	cp $(KERNEL_EFI) disk/EFI/BOOT/BOOTX64.EFI
	cp shell.bin disk/shell.bin
	qemu-system-x86_64 -nographic -bios $(OVMF_FD) -drive file=fat:rw:disk,format=raw -net none

clean:
	rm -f kernel.so $(KERNEL_EFI) $(KERNEL_OBJS) programs/shell.o programs/stub.o shell.elf shell.bin
	rm -rf disk
