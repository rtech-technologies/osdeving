CC = gcc
LD = ld
OBJCOPY = objcopy

CFLAGS = -I../include -fno-stack-protector -fpic \
         -fshort-wchar -mno-red-zone -Wall -fno-builtin -m64

LDFLAGS = -nostdlib -znocombreloc -shared -Bsymbolic

KERNEL_SRCS = ../kernel/entry.c \
              ../kernel/main.c \
              ../services/io/console.c \
              ../services/io/input.c \
              ../services/io/font_data.c \
              ../services/mem/memory.c \
              ../services/io/disk.c \
              ../services/fs/fs.c \
              ../services/fs/rnafs.c \
              ../services/core/event.c \
              ../services/system/power.c

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)

all: bootx64.efi shell.bin

bootx64.efi: kernel.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
	           -j .dynsym  -j .rel -j .rela -j .reloc \
	           --target=efi-app-x86_64 kernel.so bootx64.efi
	mkdir -p ../boot
	cp bootx64.efi ../boot/

kernel.so: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o kernel.so

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

shell.bin: ../programs/shell.c ../programs/libsystem.c ../services/io/console.o ../services/io/input.o ../services/io/font_data.o ../services/core/event.o
	$(CC) $(CFLAGS) -c ../programs/shell.c -o shell.o
	$(CC) $(CFLAGS) -c ../programs/libsystem.c -o libsystem.o
	# Category link order: libsystem.o first for entry point at 0x0
	$(LD) -nostdlib -Ttext 0x0 --oformat binary libsystem.o shell.o ../services/io/console.o ../services/io/input.o ../services/io/font_data.o ../services/core/event.o -o shell.bin
	cp shell.bin ../programs/
	mkdir -p ../boot
	cp shell.bin ../boot/

run: all
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=fat:rw:../boot -net none

clean:
	rm -f $(KERNEL_OBJS) kernel.so shell.o libsystem.o bootx64.efi shell.bin ../boot/bootx64.efi ../boot/shell.bin ../programs/shell.bin
