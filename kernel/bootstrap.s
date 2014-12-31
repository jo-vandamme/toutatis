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
PAGE_DIRECTORY          equ 0x9c000     ; page directory table, address must be 4KB aligned
PAGE_TABLE_0            equ 0x9d000     ; 0th page table, must be 4KB aligned
PAGE_TABLE_KERNEL       equ 0x9e000     ; kernel page table, must be 4KB aligned

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

        mov     esp, stack_start + STACK_SIZE
        sub     esp, kernel_voffset

        call    enable_paging

        add     esp, kernel_voffset
        mov     ebp, esp

        ; start fetching instructions in higher half
        lea     ecx, [start_in_higher_half]
        jmp     ecx

start_in_higher_half:

        ; unmap identity mapping of first 4MB.
        ;mov     dword [PAGE_DIRECTORY], 0
        ;invlpg  [0]

        ; reset EFLAGS
        push    dword 0x0
        popfd

        ; push the incoming multiboot headers
        add     ebx, kernel_voffset ; get virtual address of header pointer
        push    stack_end          
        push    stack_start       
        push    ebx                 ; header pointer
        push    eax                 ; header magic

        call    main                ; call the kernel

the_end:
        hlt
        jmp     the_end

enable_paging:
        pusha

        ; Identity map 1st page table (4MB)
        mov     eax, PAGE_TABLE_0       ; first page table
        mov     ebx, 0x00000000 | ATTR  ; first PTE
        mov     ecx, 1024               ; number of PTEs
    .identity_map:
        mov     dword [eax], ebx        ; set PTE
        add     eax, 4                  ; each entry is 4 bytes
        add     ebx, 0x1000             ; each page frame is 0x1000 bytes
        loop    .identity_map           ; loop over entire PT

        ; Map the kernel PT
        mov     eax, PAGE_TABLE_KERNEL
        mov     ebx, 0x00000000 | ATTR  ; first PTE
        mov     ecx, 1024               ; number of PTEs
    .kernel_map:
        mov     dword [eax], ebx        ; set PTE
        add     eax, 4                  ; sizeof(PTE) = 4 bytes
        add     ebx, 0x1000             ; each page frame is 0x1000 bytes
        loop    .kernel_map             ; loop over entire PT

        ; Setup entries in the directory table
        mov     eax, PAGE_TABLE_0 | ATTR    ; 1st PDE
        mov     dword [PAGE_DIRECTORY], eax ; set 1st PDE at PD physical address

        mov     eax, PAGE_TABLE_KERNEL | ATTR ; kernel PDE
        mov     ebx, kernel_voffset     ; virtual address offset
        shr     ebx, 22                 ; get high 10 bits = kernel PDE index
        shl     ebx, 2                  ; PDE is 4 bytes wide, multiply by 4
        add     ebx, PAGE_DIRECTORY     ; ebx = byte offset to the kernel PDE in the PD
        mov     dword [ebx], eax        ; set kernel PDE

        ; Install directory table and enable paging
        mov     eax, PAGE_DIRECTORY     ; PD virtual address
        mov     cr3, eax                ; set current PD

        mov     eax, cr4
        and     eax, 0xffffffef         ; set PSE (page size extension) bit to 0 (4KB pages instead of 4MB)
        mov     cr4, eax
        
        mov     eax, cr0
        or      eax, 1 << 31            ; set PG (paging enable) bit to 1
        mov     cr0, eax

        popa
        ret

section .bss
align 16

stack_start:
        resb STACK_SIZE
stack_end:
