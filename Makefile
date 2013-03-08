CC = gcc
AS = nasm
CFLAGS = -O3 -g -m32 -std=c99 -Wall -Wextra -Werror \
	 -ffreestanding -fno-builtin -nostdlib -nostdinc \
	 -nodefaultlibs -fno-leading-underscore -nostartfiles \
	 -Ikernel/
ASFLAGS = -O3 -g -felf
LDFLAGS = -melf_i386

#include boot/make.inc
include kernel/make.inc

.s.o:
	@echo "[NASM]   "$@
	@$(AS) $(ASFLAGS) -o $@ $?

.c.o:
	@echo "[CC]     "$@
	@$(CC) -c $(CFLAGS) -o $@ $?

all: clean $(KOBJS)
	@echo "[NASM]   boot/boot.bin"
	@nasm -f bin -O3 -Iboot/ -o bin/boot.bin boot/boot.s
	@echo "[NASM]   boot/loader.bin"
	@nasm -f bin -O3 -Iboot/ -o bin/loader.bin boot/loader.s
	@echo "[LD]     "kernel.elf
	@ld $(LDFLAGS) -Tkernel/link.ld $(KOBJS) -o ./bin/kernel.elf

clean:
	@rm -f $(KOBJS)
	@rm -f bin/boot.bin
	@rm -f bin/loader.bin
	@rm -f bin/kernel.elf
	@rm -f bin/sos.img

floppy: all
	@dd if=/dev/zero of=bin/sos.img bs=512 count=2880
	@mkdosfs -f 2 -F 12 bin/sos.img
	@dd if=bin/boot.bin of=bin/sos.img bs=1 skip=0  seek=0  count=3   conv=notrunc
	@dd if=bin/boot.bin of=bin/sos.img bs=1 skip=62 seek=62 count=450 conv=notrunc
	@sudo mount bin/sos.img bin/target -o loop
	@sudo cp bin/kernel.elf bin/target/
	@sudo cp bin/loader.bin bin/target/
	@sync
	@sudo umount bin/target

qemu:
	qemu-system-x86_64 -fda bin/sos.img -k en-us

bochs:
	cd bin && bochs -q -f bochsrc.txt

q: floppy qemu

b: floppy bochs
