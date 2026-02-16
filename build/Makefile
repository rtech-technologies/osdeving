CC = gcc
LD = ld
OBJCOPY = objcopy

CFLAGS = -I../include -fno-stack-protector -fpic \
         -fshort-wchar -mno-red-zone -Wall -fno-builtin -m64

LDFLAGS = -nostdlib -znocombreloc -shared -Bsymbolic

KERNEL_SRCS = ../kernel/entry.c \
              ../kernel/main.c \
              ../services/console.c \
              ../services/font_data.c \
              ../services/memory.c \
              ../services/disk.c \
              ../services/fs.c \
              ../services/rnafs.c \
              ../services/event.c

# Category 10: ACPI power code isolation - separate object
POWER_OBJ = ../services/power.o

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

shell.bin: ../programs/shell.c ../programs/libsystem.c ../services/console.o ../services/font_data.o ../services/event.o
	$(CC) $(CFLAGS) -c ../programs/shell.c -o shell.o
	$(CC) $(CFLAGS) -c ../programs/libsystem.c -o libsystem.o
	$(LD) -nostdlib -Ttext 0x0 --oformat binary shell.o libsystem.o ../services/console.o ../services/font_data.o ../services/event.o -o shell.bin
	cp shell.bin ../programs/

clean:
	rm -f $(KERNEL_OBJS) $(POWER_OBJ) kernel.so shell.o libsystem.o bootx64.efi shell.bin ../boot/bootx64.efi ../programs/shell.bin
