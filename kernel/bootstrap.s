;===============================================================================
; start.asm - starts the kernel
;===============================================================================

[bits 32]

[global bootstrap]
[extern main]
[extern kernel_start]               ; start of the '.text' section
[extern kernel_end]                 ; end of the last loadable section

section .text

MBOOT_PAGE_ALIGN        equ 1 << 0  ; load kernel and modules on a page boundary
MBOOT_MEM_INFO          equ 1 << 1  ; we want memory info
MBOOT_MEM_MAP           equ 1 << 6  ; we want a memory map
MBOOT_BOOTLOADER_NAME   equ 1 << 9  ; the boot loader's name
MBOOT_HEADER_MAGIC      equ 0x1badb002  ; multiboot magic value
MBOOT_HEADER_FLAGS      equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_MEM_MAP | MBOOT_BOOTLOADER_NAME
MBOOT_CHECKSUM          equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

align 4
multiboot_header:
        dd  MBOOT_HEADER_MAGIC      ; we are multiboot compatible
        dd  MBOOT_HEADER_FLAGS
        dd  MBOOT_CHECKSUM          ; to ensure that the above values are correct

bootstrap:
        cli
        mov     dx, 0x10            ; set data segments to data selector (0x10)
        mov     ds, dx
        mov     es, dx
        mov     fs, dx
        mov     gs, dx
        mov     ss, dx
        mov     esp, 0x90000
        mov     ebp, esp

        ; calculate kernel size
        mov     ecx, kernel_end
        sub     ecx, kernel_start

        ; push the incoming multiboot headers
        push    ecx                 ; kernel size
        push    ebx                 ; header pointer
        push    eax                 ; header magic

        call    main                ; call the kernel

        cli
        hlt
        jmp $
