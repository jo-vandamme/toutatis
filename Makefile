CC = gcc
AS = nasm
#-nostdinc
CFLAGS = -O3 -g -m32 -std=c99 -pedantic -Wall -Wextra -Werror \
     -Wno-unused-function -Wno-unused-parameter \
	 -ffreestanding -fno-builtin -nostdlib -fno-omit-frame-pointer \
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

all: initrd clean $(KOBJS)
	@echo "[LD]     kernel.elf"
	@ld $(LDFLAGS) -Tkernel/link.ld $(KOBJS) -o ./bin/kernel.elf

clean:
	@rm -f $(KOBJS)

initrd:
	@echo "[CC]     make_initrd.c"
	@$(CC) make_initrd.c -o ./bin/make_initrd
	@cd bin/initrd && ../make_initrd `find . -type f | sed 's/.\///'`
	@mv bin/initrd/initrd.img bin/iso/boot/

iso: all
	@cp bin/kernel.elf bin/iso/boot
	@gzip -c -9 bin/kernel.elf > bin/kernel.elf.zip
	@cp bin/kernel.elf.zip bin/iso/boot
	@mkisofs -input-charset utf8 -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o bin/toutatis.iso bin/iso

bochs_iso:
	@cd bin && bochs -q -f bochsrc_iso.bxrc

qemu_iso:
	@qemu-system-i386 -cdrom bin/toutatis.iso -k en-us -monitor stdio -serial /dev/tty -vga std -m 1024

isoq: iso qemu_iso

isob: iso bochs_iso

