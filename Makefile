CC = gcc
AS = nasm
CFLAGS = -O3 -g -m32 -std=c99 -Wall -Wextra -Werror \
	 -ffreestanding -fno-builtin -nostdlib -nostdinc \
	 -nodefaultlibs -fno-leading-underscore -nostartfiles \
	 -Ikernel/
ASFLAGS = -O3 -g -felf
LDFLAGS = -melf_i386 -nostdlib -nostartfiles -nostdinc -nodefaultlibs

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

floppy: all
	@dd if=/dev/zero of=bin/toutatis.img bs=512 count=2880
	@mkdosfs -f 2 -F 12 bin/toutatis.img
	@dd if=bin/boot.bin of=bin/toutatis.img bs=1 skip=0  seek=0  count=3   conv=notrunc
	@dd if=bin/boot.bin of=bin/toutatis.img bs=1 skip=62 seek=62 count=450 conv=notrunc
	@sudo mount bin/toutatis.img bin/target -o loop
	@sudo cp bin/kernel.elf bin/target/
	@sudo cp bin/loader.bin bin/target/
	@sync
	@sudo umount bin/target

iso: all
	@cp /boot/grub/stage2_eltorito bin/iso/boot/grub
	@cp bin/menu.lst bin/iso/boot/grub
	@cp bin/kernel.elf bin/iso/boot
	@gzip -c -9 bin/kernel.elf > bin/kernel.elf.zip
	@cp bin/kernel.elf.zip bin/iso/boot
	@mkisofs -input-charset utf8 -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o bin/toutatis.iso bin/iso

qemu:
	qemu-system-x86_64 -fda bin/toutatis.img -k en-us -monitor stdio -serial /dev/tty -vga std -m 128

bochs:
	cd bin && bochs -q -f bochsrc.txt

bochs_iso:
	cd bin && bochs -q -f bochsrc_iso.txt

qemu_iso:
	#qemu-system-x86_64 -cdrom bin/toutatis.iso -k en-us -monitor stdio -serial /dev/tty -vga std -m 128
	qemu-system-x86_64 -kernel bin/kernel.elf -k en-us -monitor stdio -serial /dev/tty -vga std -m 128

q: floppy qemu

b: floppy bochs

qg: iso qemu_iso

bg: iso bochs_iso
