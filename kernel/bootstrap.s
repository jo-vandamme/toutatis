;===============================================================================
; bootstrap.asm - starts the kernel
;===============================================================================

[bits 32]

[global bootstrap]
[extern main]
[extern kernel_start]                   ; start of the '.text' section
[extern kernel_end]                     ; end of the last loadable section
[extern kernel_voffset]

; linker entry point
bootstrap               equ (_bootstrap - 0xc0000000)

MBOOT_PAGE_ALIGN        equ 1 << 0      ; load kernel and modules on a page boundary
MBOOT_MEM_INFO          equ 1 << 1      ; we want memory info
MBOOT_HEADER_MAGIC      equ 0x1badb002  ; multiboot magic value
MBOOT_HEADER_FLAGS      equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM          equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)
STACK_SIZE              equ 0x4000      ; 16k stack
ATTR                    equ 3           ; present | writable | supervisor mode
kernel_pd               equ 0x9c000
kernel_pt               equ 0x9d000
identity_pt             equ 0x9e000

section .text
align 4

multiboot_header:
        dd  MBOOT_HEADER_MAGIC      ; we are multiboot compatible
        dd  MBOOT_HEADER_FLAGS      ; how the bootloader will boot us
        dd  MBOOT_CHECKSUM          ; to ensure that the above values are correct

_bootstrap:
        cli
        mov     dx, 0x10            ; set data segments to data selector (0x10)
        mov     ds, dx
        mov     es, dx
        mov     fs, dx
        mov     gs, dx
        mov     ss, dx

        mov     esp, stack + STACK_SIZE
        sub     esp, kernel_voffset

        call    enable_paging

        add     esp, kernel_voffset
        mov     ebp, esp

        ; start fetching instructions in higher half
        lea     ecx, [start_in_higher_half]
        jmp     ecx

start_in_higher_half:

        ; unmap identity mapping of first 4MB.
        mov     dword [kernel_pd], 0
        invlpg  [0]

        ; reset EFLAGS
        push    dword 0x0
        popfd

        ; calculate kernel size
        mov     ecx, kernel_end
        sub     ecx, kernel_start

        ; push the incoming multiboot headers
        add     ebx, kernel_voffset ; get virtual address of header pointer
        push    ecx                 ; kernel size
        push    ebx                 ; header pointer
        push    eax                 ; header magic

        call    main                ; call the kernel

the_end:
        hlt
        jmp     the_end

enable_paging:
        pusha

        ; Identity map 1st page table (4MB)
        mov     eax, identity_pt        ; first page table
        mov     ebx, 0x00000000 | ATTR  ; first PTE
        mov     ecx, 1024               ; number of PTEs
    .id_map:
        mov     dword [eax], ebx        ; set PTE
        add     eax, 4                  ; each entry is 4 bytes
        add     ebx, 0x1000             ; each page frame is 0x1000 bytes
        loop    .id_map                 ; loop over entire PT

        ; Map the kernel PT
        mov     eax, kernel_pt
        mov     ebx, 0x00000000 | ATTR  ; first PTE
        mov     ecx, 1024               ; number of PTEs
    .map_kernel:
        mov     dword [eax], ebx        ; set PTE
        add     eax, 4                  ; sizeof(PTE) = 4 bytes
        add     ebx, 0x1000             ; each page frame is 0x1000 bytes
        loop    .map_kernel             ; loop over entire PT

        ; Setup entries in the directory table
        mov     eax, identity_pt        ; 1st PDE
        or      eax, ATTR               ; set attributes
        mov     ebx, kernel_pd          ; PD physical address
        mov     dword [ebx], eax        ; set 1st PDE

        mov     eax, kernel_pt          ; kernel PDE
        or      eax, ATTR               ; set attributes
        mov     ebx, kernel_voffset     ; virtual address offset
        shr     ebx, 22                 ; get last 10 bits = kernel PDE index
        shl     ebx, 2                  ; PDE is 4 bytes wide, multiply by 4
        add     ebx, kernel_pd          ; ebx = byte offset to the kernel PDE in the PD
        mov     dword [ebx], eax        ; set kernel PDE

        ; Install directory table and enable paging
        mov     eax, kernel_pd          ; PD virtual address
        mov     cr3, eax                ; set current PD
        mov     eax, cr0
        or      eax, 1 << 31            ; enable paging (set bit 31)
        mov     cr0, eax

        popa
        ret

section .bss
align 32

stack:
        resb STACK_SIZE
