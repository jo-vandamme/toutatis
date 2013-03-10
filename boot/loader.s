;===============================================================================
; Kernel loader
; Loaded at 0x500 (0050:0000)
;
;   x86 Memory map
;   --------------
;
;   0x00000000 - 0x000003ff -> Interrupt Vector Table (IVT)
;   0x00000400 - 0x000004ff -> BIOS Data Area (BDA)
;   0x00000500 - 0x00007bff -> Kernel loader (~29 KiB)
;   0x00007c00 - 0x00007dff -> Bootsector (512 bytes)
;   0x00007e00 - 0x0000adff -> FAT (12 KiB)
;   0x0000ae00 - 0x0000ddff -> Root directory (12 KiB)
;   0x0000de00 - 0x0000ffff -> Stack (~8 KiB)
;   0x00010000 - 0x0009fbff -> Kernel (real mode)
;   0x0009fc00 - 0x0009ffff -> Extended BIOS Data Area (EBDA)
;   0x000a0000 - 0x000bffff -> Video RAM (VRAM) Memory
;   0x000b0000 - 0x000b7777 -> Monochrome video memory
;   0x000b8000 - 0x000bffff -> Color text video memory
;   0x000c0000 - 0x000c7fff -> Video ROM BIOS
;   0x000c8000 - 0x000effff -> BIOS Shadow Area (Mapped hardware & misc.)
;   0x000f0000 - 0x000fffff -> System BIOS (ROM)
;   0x00100000 - 0x0010ffef -> Kernel (protected mode)
;
;===============================================================================

[bits 16]
[org 0x500]

jmp stage2

%include "common.inc"
%include "stdio16.inc"
%include "stdio32.inc"
%include "gdt.inc"
%include "a20.inc"
%include "fat12.inc"
%include "multiboot.inc"
%include "memory.inc"

MEMORY_MAP_ADDRESS      equ     0x1000

;-------------------------------------------------------------------------------
; Data
;-------------------------------------------------------------------------------

s2Msg           db 0x0a, "-> ", 0
gdtMsg          db "Installing GDT", 0
a20Msg          db "Enabling A20 line", 0
rLoadMsg        db "Loading kernel to low memory", 0
pmodeMsg        db "Switching to protected mode", 0
loadErrorMsg    db "Missing or corrupt kernel.elf Press any key to reboot", 0
memErrorMsg     db "Can't detect memory", 0
kernelName      db KERNEL_NAME, 0
loaderName      db BOOTLOADER_NAME, 0

boot_info:
istruc multiboot_info
        at multiboot_info.flags,                 dd 0
        at multiboot_info.memory_lower,          dd 0
        at multiboot_info.memory_upper,          dd 0
        at multiboot_info.boot_device,           dd 0
        at multiboot_info.cmd_line,              dd 0
        at multiboot_info.mods_count,            dd 0
        at multiboot_info.mods_addr,             dd 0
        at multiboot_info.syms0,                 dd 0
        at multiboot_info.syms1,                 dd 0
        at multiboot_info.syms2,                 dd 0
        at multiboot_info.syms3,                 dd 0
        at multiboot_info.mmap_length,           dd 0
        at multiboot_info.mmap_addr,             dd 0
        at multiboot_info.drives_length,         dd 0
        at multiboot_info.drives_addr,           dd 0
        at multiboot_info.config_table,          dd 0
        at multiboot_info.boot_loader_name,      dd 0
        at multiboot_info.apm_table,             dd 0
        at multiboot_info.vbe_control_info,      dd 0
        at multiboot_info.vbe_mode_info,         dd 0
        at multiboot_info.vbe_mode,              dw 0
        at multiboot_info.vbe_interface_seg,     dw 0
        at multiboot_info.vbe_interface_off,     dw 0
        at multiboot_info.vbe_interface_len,     dw 0
iend

;-------------------------------------------------------------------------------
; Stage 2 entry point
;   - store BIOS info
;   - load kernel
;   - go in protected mode
;   - jump to stage 3
;-------------------------------------------------------------------------------

stage2:

        cld
        xor     ax, ax
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax

        cli
        mov     ss, ax
        mov     sp, 0xffff
        sti

        mov     byte [boot_info + multiboot_info.boot_device], dl
        mov     word [boot_info + multiboot_info.boot_loader_name], loaderName
        or      dword [boot_info + multiboot_info.flags], MULTIBOOT_BOOTDEV
        or      dword [boot_info + multiboot_info.flags], MULTIBOOT_LOADER

        ; Make sure we print at (0,0)
        mov     byte [XPOS], 0
        mov     byte [YPOS], 0

        puts16  s2Msg, S2ATT
        puts16  gdtMsg, TEXTATT

        ; Install GDT
        call    installGDT

        puts16  s2Msg, S2ATT
        puts16  a20Msg, TEXTATT

        ; Enable A20
        call    enableA20

        ; get lower memory size
        call    get_low_memory_size
        cmp     ax, -1
        je      .mem_error
        mov     word [boot_info + multiboot_info.memory_lower], ax ; number of KB for lower memory

        ; get upper memory size
        xor     eax, eax
        xor     ebx, ebx
        call    get_high_memory_size
        cmp     ax, -1
        je      .mem_error
        mov     word [mem1to16], ax
        mov     word [memAbove16], bx

        ; get the memory map
        xor     eax, eax
        mov     ax, MMAP_SEG
        mov     ds, ax
        mov     di, MMAP_OFF
        call    get_memory_map
        jc      .mem_error
        xor     eax, eax
        mov     ax, bp
        mov     bl, 24          ; a memory map entry is 24 bytes long
        mul     bl
        mov     dword [boot_info + multiboot_info.mmap_addr], MMAP_SEG * 16 + MMAP_OFF
        mov     word [boot_info + multiboot_info.mmap_length], ax
        or      dword [boot_info + multiboot_info.flags], MULTIBOOT_MMAP

        jmp     .load_kernel
    .mem_error:
        puts16  s2Msg, S2ATT
        puts16  memErrorMsg, ERRATT
        jmp     .stop

    .load_kernel:
        puts16  s2Msg, S2ATT
        puts16  rLoadMsg, TEXTATT
        ;puts16  s2Msg, S2ATT

        ; Load the kernel
        mov     si, kernelName
        call    find_file
        cmp     ax, -1
        je      .load_error

        push    KERNEL_RSEG
        pop     gs
        mov     bx, KERNEL_ROFF
        call    load_file

        mov     dword [KERNEL_SIZE], ecx
        jmp     enter_pmode

    .load_error:
        puts16  s2Msg, S2ATT
        puts16  loadErrorMsg, ERRATT

    .stop:
        mov     ah, 0
        int     0x16                        ; await keypress
        int     0x19                        ; warm boot computer

        cli                                 ; if we get here, something really went wrong
        hlt

enter_pmode:

        puts16  s2Msg, S2ATT
        puts16  pmodeMsg, TEXTATT

        ; Go into PMode
        cli
        mov     eax, cr0                    ; set bit 0 in cr0 to enter PMode
        or      eax, 1
        mov     cr0, eax

        jmp     CODE_DESC:pmode             ; far jump to fix cs. The code selector is 0x08

        ; Do not re-enable interrupts! Doing this will triple fault!
        ; We will fix this in stage 3.

;-------------------------------------------------------------------------------
; Stage 3 entry point
;-------------------------------------------------------------------------------

[bits 32]

pmode:

        ; Set registers
        mov     ax, DATA_DESC               ; set data segments to data selector (0x10)
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        mov     ss, ax
        mov     esp, 0x90000                ; stack begins at 0x90000
        mov     ebp, esp                    ; C uses ebp as a frame pointer

        ; turn off floppy motor
        mov     edx, 0x3f2
        mov     al, 0x0c
        out     dx, al

        puts32  s3Msg, S3ATT
        puts32  pLoadMsg, TEXTATT

        ; Calculate upper memory size
        xor     eax, eax
        xor     ebx, ebx
        mov     ax, word [mem1to16]
        mov     bx, word [memAbove16]
        shl     ebx, 6          ; multiply by 64
        add     eax, ebx
        mov     dword [boot_info + multiboot_info.memory_upper], eax ; number of KB for upper memory
        or      dword [boot_info + multiboot_info.flags], MULTIBOOT_MEMINFO

        ; convert the ELF file to a linear binary so we can execute it
        ; unpack the ELF into where it needs to go

load_elf:
        ; http://www.codewiki.wikispaces.com/elf_load.nasm
        ; http://www.linuxjournal.com/article/1059
        ; validate that it is an ELF file
        cmp     dword [KERNEL_RBASE], 0x464c457f    ; the ELF signature is \07fELF
        jne     .elf_error
        cmp     byte [KERNEL_RBASE + 0x4], 1        ; 32 bit
        jne     .elf_error
        cmp     word [KERNEL_RBASE + 0x10], 2       ; executable file
        jne     .elf_error
        cmp     word [KERNEL_RBASE + 0x12], 3       ; Intel 80386 machine
        jne     .elf_error
        jmp     short .skip_err_handler

    .elf_error:
        puts32  s3Msg, S3ATT
        puts32  elfErrMsg, ERRATT
        mov     ah, 0
        int     0x16                                ; await keypress
        int     0x19                                ; warm boot computer

    .skip_err_handler:
        mov     eax, dword [KERNEL_RBASE + 0x18]    ; entry point address
        mov     [kernel_entry], eax
        movzx   ecx, word [KERNEL_RBASE + 0x2c]     ; number of segments

    .section_loop:
        dec     cx                                  ; next segment
        push    cx                                  ; save cx on the stack while we load
                                                    ; the segment into memory
        movzx   eax, word [KERNEL_RBASE + 0x2a]     ; program header entry size
        mul     cx                                  ; offset from the start of the program header table

        mov     ebx, dword [KERNEL_RBASE + 0x1c]    ; program header table offset
        add     ebx, eax                            ; program header entry offset from start of file
        add     ebx, KERNEL_RBASE                   ; address of the program header table entry
        cmp     dword [ebx], 1                      ; the segment is loadable
        jne     .next_section                       ; the segment isn't loadable

        mov     ecx, dword [ebx + 0x4]              ; segment offset in file
        mov     ebp, dword [ebx + 0x10]             ; segment size in file
        mov     edi, dword [ebx + 0x8]              ; segment memory address
        mov     eax, dword [ebx + 0x14]             ; segment size in memory
        mov     ebx, eax                            ; segment size in memory
        push    ebp                                 ; segment size in file
        pusha
        mov     esi, KERNEL_RBASE
        add     esi, ecx                            ; segment base address
        mov     ecx, ebp                            ; segment size in file
        call    memcopy
        popa
        pop     eax                                 ; segment size in file
        sub     ebx, eax                            ; number of bytes to be zeroed
        jz      .next_section

        add     edi, eax                            ; zero the memory from this address
        xor     ax, ax                              ; we want to zero out the memory
        mov     ecx, ebx                            ; number of bytes to zero out
        call    memset

    .next_section:
        pop     cx
        or      cx, cx
        jnz     .section_loop

        puts32  s3Msg, S3ATT
        puts32  execMsg, TEXTATT

        push    dword 2                             ; go in with a nice clean set of flags
        popfd

        ; XXX: bug, without this newline32, the cursor position
        ; isn't properly set and the kernel overwrites the bootloader messages
        newline32
        ; set the cursor position before jumping to the kernel
        mov     bh, byte [YPOS]
        mov     bl, byte [XPOS]
        call    move_cursor

        ; the kernel is now loaded into high memory
        mov     ecx, [kernel_entry]
        mov     eax, MULTIBOOT_MAGIC                ; multiboot header magic number
        mov     ebx, boot_info                      ; multiboot header pointer
        call    ecx                                 ; execute the kernel

        cli                                         ; we should not get here
        hlt                                         ; but in case, halt the cpu
        jmp $                                       ; in case of a stray interrupt

;-------------------------------------------------------------------------------
; memcopy: copies ecx bytes from esi to edi
; esi -> source address
; edi -> destination address
; ecx -> number of bytes to copy
;-------------------------------------------------------------------------------

memcopy:
        pusha
        .memcopy_loop:
            mov     al, [esi]
            mov     [edi], al
            inc     edi
            inc     esi
            loop    .memcopy_loop
        popa
        ret

;-------------------------------------------------------------------------------
; memset: copies byte al ecx times beginning at address pointed by edi
; edi -> start of memory block to be set
; ecx -> number of bytes to set
; al  -> byte to copy
;-------------------------------------------------------------------------------

memset:
        push    edi
        push    ecx
        cld                                 ; clear the direction flag
        a32     rep stosb                   ; fill the memory one byte at a time
        pop     ecx
        pop     edi
        ret

;-------------------------------------------------------------------------------
; data
;-------------------------------------------------------------------------------

mem1to16        dw  0x0000
memAbove16      dw  0x0000
kernel_entry    dd  0x00000000
s3Msg           db  0x0a, "-> ", 0
pLoadMsg        db  "Unpacking ELF kernel to high memory", 0
execMsg         db  "Transferring control to kernel", 0
elfErrMsg       db  "Invalid ELF file", 0
