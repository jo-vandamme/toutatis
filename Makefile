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
	@echo "[LD]     "kernel.elf
	@ld $(LDFLAGS) -Tkernel/link.ld $(KOBJS) -o ./bin/kernel.elf

clean:
	@rm -f $(KOBJS)

iso: all
	#@cp /boot/grub/stage2_eltorito bin/iso/boot/grub
	#@cp bin/stage2_eltorito bin/iso/boot/grub
	#@cp bin/menu.lst bin/iso/boot/grub
	@cp bin/kernel.elf bin/iso/boot
	@gzip -c -9 bin/kernel.elf > bin/kernel.elf.zip
	@cp bin/kernel.elf.zip bin/iso/boot
	@mkisofs -input-charset utf8 -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o bin/toutatis.iso bin/iso

bochs_iso:
	@cd bin && bochs -q -f bochsrc_iso.bxrc

qemu_iso:
	@qemu-system-x86_64 -cdrom bin/toutatis.iso -k en-us -monitor stdio -serial /dev/tty -vga std -m 128

isoq: iso qemu_iso

isob: iso bochs_iso

#@qemu-system-x86_64 -kernel bin/kernel.elf -k en-us -monitor stdio -serial /dev/tty -vga std -m 128
#qemu-system-x86_64 -cdrom bin/toutatis.iso -k en-us -monitor stdio -serial /dev/tty -vga std -m 128
#qemu-system-x86_64 -kernel bin/kernel.elf -k en-us -monitor stdio -serial file:./bin/com1.out -vga std -m 128

