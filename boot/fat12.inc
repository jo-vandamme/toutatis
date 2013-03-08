%ifndef __FAT12_INC_INCLUDED__
%define __FAT12_INC_INCLUDED__

[bits 16]

%include "common.inc"
%include "stdio16.inc"

cluster dw 0x0000

;-------------------------------------------------------------------------------
; load_file: Loads a file into memory
; di    -> file's root directory entry
; gs:bx -> buffer to load file to
; cx    <- number of sectors read
;-------------------------------------------------------------------------------

load_file:
        ;putc16  'L', TEXTATT
        xor     cx, cx
        push    cx
        push    bx

        mov     dx, word [di + 0x001a]          ; first file's cluster
        mov     word [cluster], dx

    .load:
        ; read the cluster
        mov     ax, word [cluster]              ; cluster to read
        call    cluster_to_LBA                  ; convert cluster to LBA
        xor     cx, cx
        mov     cl, byte [sectPerClust]         ; sectors to read
        pop     bx                              ; buffer to read into
        call    read_sectors
        pop     cx
        inc     cx
        push    cx
        push    bx

        ; compute next cluster
        mov     ax, word [cluster]              ; identify current cluster
        mov     cx, ax
        mov     dx, ax
        shr     dx, 1
        add     cx, dx
        mov     bx, FATBUF
        add     bx, cx
        mov     dx, word [es:bx]
        test    ax, 0x0001
        jnz     .odd_cluster

        .even_cluster:
            and     dx, 0x0fff
            jmp     .done

        .odd_cluster:
            shr     dx, 0x0004

        .done:
            mov     word [cluster], dx
            cmp     dx, 0x0ff0
            jb      .load

        .success:
            pop     bx
            pop     cx
            ret

;-------------------------------------------------------------------------------
; find_file: Searches for filename in root table
; ds:si -> filename
; ax    <- file index number in directory table. -1 if error
; di    <- address of the file's entry in the root directory table
;-------------------------------------------------------------------------------

find_file:
        ;putc16 'S', TEXTATT
        push    cx
        push    bx
        mov     bx, si                          ; copy filename for later

        ; browse root directory for file
        mov     cx, word [nRootDirEnts]         ; loop counter
        mov     di, ROOTDIR
        cld

    .loop:
        ;putc16  '.', TEXTATT
        push    cx
        mov     cx, 11                          ; 11 character names
        mov     si, bx
        push    di
        rep     cmpsb
        pop     di
        je      .found
        pop     cx
        add     di, 32                          ; queue next directory entry
        loop    .loop
        putc16  'Q', ERRATT

    .not_found:
        pop     bx
        pop     cx
        mov     ax, -1
        ret

    .found:
        pop     ax                              ; file index number in directory table
        pop     bx
        pop     cx
        ret

;-------------------------------------------------------------------------------
; read_sectors: reads a series of sectors
; cx    -> number of sectors to read
; ax    -> starting sector
; gs:bx -> output buffer
;-------------------------------------------------------------------------------

read_sectors:
        push    di
    .main:
        mov     di, 0x0005                      ; five retries for error
    .sector_loop:
        push    ax
        push    bx
        push    cx
        call    LBA_to_CHS                      ; convert starting sector to CHS
        push    es                              ; exchange es and gs
        push    gs
        pop     es
        mov     ah, 0x02                        ; BIOS read sector
        mov     al, 0x01                        ; read one sector
        stc                                     ; some BIOS do net set the flag correct on error
        int     0x13                            ; invoke BIOS
        pop     es
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
        ;putc16  '.', TEXTATT
        pop     cx
        pop     bx
        pop     ax
        add     bx, word [bytesPerSect]         ; queue next buffer
        inc     ax                              ; queue next sector
        loop    .main                           ; read next sector
        pop     di
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
        push    cx

        sub     ax, 0x0002                      ; zero base cluster number
        xor     cx, cx
        mov     cl, byte [sectPerClust]         ; convert byte to word
        mul     cx
        add     ax, word [DATA_SECTOR]          ; base data sector

        pop     cx
        ret

%endif ;__FAT12_INC_INCLUDED__
