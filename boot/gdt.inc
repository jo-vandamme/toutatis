;===============================================================================
; gdt.inc
;===============================================================================

%ifndef __GDT_INC_INCLUDED__
%define __GDT_INC_INCLUDED__

%define NULL_DESC 0x00
%define CODE_DESC 0x08
%define DATA_DESC 0x10

[bits 16]

;-------------------------------------------------------------------------------
; installGDT
;-------------------------------------------------------------------------------

installGDT:

        pusha

        cli
        lgdt [gdtp]                     ; load GDT into GDTR
        sti

        popa
        ret

;-------------------------------------------------------------------------------
; GDT
;-------------------------------------------------------------------------------

; Define read and write permissions from the first byte to byte 0xffffffff in memory

gdt_data:

    ; Null descriptor - offset 0
        dd  0                           ; null descriptor -- just fill 8 bytes with 0
        dd  0

    ; Kernel space code descriptor - offset 0x8 bytes from start of GDT.
        dw  0xffff                      ; limit 0:15  bits
        dw  0x0000                      ; base  0:15  bits
        db  0x00                        ; base  16:23 bits
        db  10011010b                   ; Pr (must be 1)-Privl (2 bits)-1-Ex-DC-RW-AC
        db  11001111b                   ; Gr-Sz-0-0 - limit 16:19 bits
        db  0x00                        ; base  24:31 bits

    ; Kernel space data descriptor - offset 16 bytes (0x10) from start of GDT.
        dw  0xffff                      ; limit 0:15  bits
        dw  0x0000                      ; base  0:15  bits
        db  0x00                        ; base  16:23 bits
        db  10010010b                   ; Pr (must be 1)-Privl (2 bits)-1-Ex-DC-RW-AC
        db  11001111b                   ; Gr-Sz-0-0 - limit 16:19 bits
        db  0x00                        ; base  24:31 bits

    ; User space code descriptor - offset 24 bytes (0x18) from start of GDT.
        dw  0xffff                      ; limit 0:15  bits
        dw  0x0000                      ; base  0:15  bits
        db  0x00                        ; base  16:23 bits
        db  11111010b                   ; Pr (must be 1)-Privl (2 bits)-1-Ex-DC-RW-AC
        db  11001111b                   ; Gr-Sz-0-0 - limit 16:19 bits
        db  0x00                        ; base  24:31 bits

    ; User space data descriptor - offset 32 bytes (0x20) from start of GDT.
        dw  0xffff                      ; limit 0:15  bits
        dw  0x0000                      ; base  0:15  bits
        db  0x00                        ; base  16:23 bits
        db  11110010b                   ; Pr (must be 1)-Privl (2 bits)-1-Ex-DC-RW-AC
        db  11001111b                   ; Gr-Sz-0-0 - limit 16:19 bits
        db  0x00                        ; base  24:31 bits

end_of_gdt:

gdtp:
        dw  end_of_gdt - gdt_data - 1   ; size of GDT
        dd  gdt_data                    ; base of GDT

%endif ;__GDT_INC_INCLUDED__
