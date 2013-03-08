;===============================================================================
; SOS Bootsector
; Loads "loader.bin" into memory at 0x0500
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

%macro putc 1
        push    ax
        mov     al, %1
        call    print_char
        pop     ax
%endmacro

[bits 16]                                       ; we need 16-bit intructions for Real mode
[org 0x7c00]

%include "common.inc"

jmp short stage1                                ; standard jump
nop                                             ; standard filler

;-------------------------------------------------------------------------------
; BIOS Parameter Block (BPB) - instead of zero-fill, initialize BPB with values
; for a 1440 K floppy
;-------------------------------------------------------------------------------

        db 'SOS BOOT'                           ; OEM label (8 characters)
        dw 512                                  ; bytes per sector
        db 1                                    ; sectors per cluster
        dw 1                                    ; reserved sectors (boot sector)
        db 2                                    ; number of FATs
        dw 224                                  ; root directory entries 224 * 32 bytes each = 7186 bytes = 14 sectors
        dw 80 * 36                              ; total sectors on disk
        db 0xf0                                 ; media descriptor
        dw 9                                    ; sectors per 1 FAT copy
        dw 18                                   ; sectors per track
        dw 2                                    ; number of heads
        dd 0                                    ; hidden sectors
        dd 0                                    ; big total sectors
        db 0                                    ; boot unit
        db 0                                    ; reserved
        db 0x29                                 ; extended boot record id
        dd 0x12345678                           ; volume serial number
        db 'SOS v0.01  '                        ; volume label (11 characters)
        db 'FAT12   '                           ; filesystem id (8 characters)

;-------------------------------------------------------------------------------
; Bootsector entry point
;-------------------------------------------------------------------------------

stage1:

    ;----------------------------------------------------
    ; code located at 7c00:0000, adjust segment registers
    ;----------------------------------------------------

        cld                                     ; all string operations increment
        xor     ax, ax
        mov     ds, ax                          ; setup segments to insure they are 0. Remember that
        mov     es, ax                          ; we have ORG 0x7c00. This means all addresses are based
        mov     fs, ax                          ; from 0x7c00:0. Because the data segments are within the same
        mov     gs, ax                          ; code segment, null em.

        cli                                     ; disable interrupts while changing stack
        mov     ss, ax
        mov     sp, 0xffff                      ; set the stack just under where we are loaded
        sti                                     ; restore interrupts

        ; a few early BIOSes are reported to improperly set dl
        mov     byte [drive], dl                ; rely on BIOS drive number stored in dl
        mov     eax, 0                          ; needed for some older bioses

    ; make sure the screen is set to 80x25 color text mode
        mov     ax, 0x0003                      ; set to normal (80x25 text) video mode
        int     0x10

        mov     si, s1Msg
        call    print_string

    ;----------------------------------------------------
    ; Load root directory table
    ;----------------------------------------------------

        putc    'R'                            ; we are about to load the root directory

    ; compute the size of root directory and store in cx
    ; cx = (rootDirEntries * 32 + bytesPerSector - 1) / bytesPerSector
        xor     cx, cx
        xor     dx, dx
        mov     ax, 0x0020                      ; 32 bytes per directory entry
        mul     word [nRootDirEnts]
        div     word [bytesPerSect]
        xchg    ax, cx

    ; compute location of root directory
        xor     ax, ax
        mov     al, byte [nFATs]                ; number of FATs
        mul     word [sectPerFAT]               ; sectors used by FATs
        mov     word [fatSize], ax
        add     ax, word [nResSectors]          ; adjust for bootsector
        mov     word [DATA_SECTOR], ax          ; base of root directory
        add     word [DATA_SECTOR], cx

    ; read root directory into memory
        mov     bx, ROOTDIR                     ; copy root dir above bootsector
        call    read_sectors

    ;----------------------------------------------------
    ; Find stage 2
    ;----------------------------------------------------

        putc    'S'                            ; we are about to search the file

    ; browse root directory for binary image
        mov     cx, word [nRootDirEnts]         ; load loop counter
        mov     di, ROOTDIR                     ; locate first root entry
    .loop:
        putc    '.'
        push    cx
        mov     cx, 11                          ; 11 character name
        mov     si, loaderFile                  ; file name to find
        push    di
        rep     cmpsb                           ; test for entry match
        pop     di
        je      load_FAT
        pop     cx
        add     di, 32                          ; queue next directory entry
        loop    .loop
        putc    'Q'
        jmp     reboot

    ;----------------------------------------------------
    ; Load FAT
    ;----------------------------------------------------

    load_FAT:

        putc    'F'                            ; we are about to load the FAT

    ; save starting cluster of boot image
        mov     dx, word [di + 0x001a]          ; file's first cluster
        mov     word [cluster], dx

    ; store FAT size in cx and FAT location in ax
        mov     cx, word [fatSize]
        mov     ax, word [nResSectors]          ; adjust for bootsector

    ; read FAT into memory
        mov     bx, FATBUF                      ; copy FAT above bootcode
        call    read_sectors

    ;----------------------------------------------------
    ; Load Stage 2
    ;----------------------------------------------------

        putc    'L'                            ; we are about to load the file

    ; read image file into memory (STAGE2_SEG:STAGE2_OFF)

        mov     ax, STAGE2_SEG                  ; setup the output buffer es:bx
        mov     es, ax
        mov     bx, STAGE2_OFF
        push    bx

    load_image:

    ; read the cluster
        mov     ax, word [cluster]              ; cluster to read
        pop     bx                              ; buffer to read into
        call    cluster_to_LBA                  ; convert cluster to LBA
        xor     cx, cx
        mov     cl, byte [sectPerClust]         ; sectors to read
        call    read_sectors
        push    bx

    ; compute next cluster
        mov     ax, word [cluster]              ; identify current cluster
        mov     cx, ax                          ; copy current cluster
        mov     dx, ax                          ; copy current cluster
        shr     dx, 0x0001                      ; divide by two
        add     cx, dx                          ; sum for (3/2)
        mov     bx, FATBUF                      ; location of FAT in memory
        add     bx, cx
        mov     dx, word [bx]                   ; read two bytes from FAT
        test    ax, 0x0001
        jnz     .odd_cluster

        .even_cluster:
            and     dx, 0x0fff                  ; take low twelve bits
            jmp     .done

        .odd_cluster:
            shr     dx, 0x0004                  ; take high twelve bits

        .done:
            mov     word [cluster], dx          ; store new cluster
            cmp     dx, 0x0ff0                  ; test for end of file
            jb      load_image

    done:

        putc    'X'

        ;call    kill_motor
        pop     ax
        mov     dl, byte [drive]

        jmp     STAGE2_SEG:STAGE2_OFF

;-------------------------------------------------------------------------------
; kill_motor: turns off the floppy motor
;-------------------------------------------------------------------------------

kill_motor:
        pusha
        xor     al, al
        mov     dx, 0x3f2
        out     dx, al
        popa
        ret

;-------------------------------------------------------------------------------
; reboot: waits for user input and reboots the computer
; (using bios int function)
;-------------------------------------------------------------------------------

reboot:
        mov     ah, 0x00
        int     0x16                            ; await keypress
        int     0x19                            ; warm reboot
        jmp     $                               ; just in case something goes wrong hang

;-------------------------------------------------------------------------------
; read_sectors: reads a series of sectors
; cx -> number of sectors to read
; ax -> starting sector
; es:bx -> output buffer
;-------------------------------------------------------------------------------

read_sectors:
    .main:
        mov     di, 0x0005                      ; five retries for error
    .sector_loop:
        push    ax
        push    bx
        push    cx
        call    LBA_to_CHS                      ; convert starting sector to CHS
        mov     ah, 0x02                        ; BIOS read sector
        mov     al, 0x01                        ; read one sector
        stc                                     ; some BIOS do net set the flag correct on error
        int     0x13                            ; invoke BIOS
        jnc     .success                        ; test for read error
        xor     ax, ax                          ; BIOS reset disk
        stc                                     ; some BIOS do net set the flag correct on error
        int     0x13                            ; invoke BIOS
        dec     di                              ; decrement error counter
        pop     cx
        pop     bx
        pop     ax
        jnz     .sector_loop                    ; attempt to read again
        int     0x18                            ; execute BASIC
    .success:
        putc    '.'
        pop     cx
        pop     bx
        pop     ax
        add     bx, word [bytesPerSect]         ; queue next buffer
        inc     ax                              ; queue next sector
        loop    .main                           ; read next sector
        ret

;-------------------------------------------------------------------------------
; LBA_to_CHS: Convert LBA to CHS
; ax -> LBA address to convert
; ch = track/cyl # = logical sector / (sectors per track * number of heads)
; cl = sector #    = (logical sector % sectors per track) + 1
; dh = head #      = (logical sector / sectors per track) % number of heads
; dl = drive #
;-------------------------------------------------------------------------------

LBA_to_CHS:
        push    ax

        xor     dx, dx
        div     word [sectPerTrack]             ; divide ax by src, remainder in dx
        add     dl,  [nResSectors]              ; account for the boot sector
        mov     cl, dl                          ; sector # goes in cl
        xor     dx, dx
        div     word [nHeads]
        mov     dh, dl                          ; head # goes in dh
        mov     ch, al                          ; track # goes is al
        mov     dl, byte [drive]                ; drive # goes in dl

        pop     ax
        ret

;-------------------------------------------------------------------------------
; cluster_to_LBA: converts a cluster number to LBA
; LBA = (cluster - 2) * sectors per cluster + data sector
; ax -> cluster number
; ax <- LBA address
;-------------------------------------------------------------------------------

cluster_to_LBA:
        sub     ax, 0x0002                      ; zero base cluster number
        xor     cx, cx
        mov     cl, byte [sectPerClust]         ; convert byte to word
        mul     cx
        add     ax, word [DATA_SECTOR]          ; base data sector
        ret

;-------------------------------------------------------------------------------
; print_char: prints a single character using int10, 0x0e function (teletype)
; al -> character to print
;-------------------------------------------------------------------------------

print_char:
        push    bx
        mov     bx, 0x0007
        mov     ah, 0x0e
        int     0x10
        pop     bx
        ret

print_string:
        pusha
    .print:
        lodsb
        or      al, al
        jz      .done
        call    print_char
        jmp     short .print
    .done:
        popa
        ret

;-------------------------------------------------------------------------------
; Data
;-------------------------------------------------------------------------------

loaderFile      db STAGE2_NAME
s1Msg           db "-> ", 0
fatSize         dw 0x0000
cluster         dw 0x0000                       ; cluster of the file we want to load

times 510 - ($ - $$) db 0                       ; we have to be 512 bytes. clear the rest of the bytes with 0
dw 0xaa55                                       ; boot signature
