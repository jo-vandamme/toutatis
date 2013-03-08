;===============================================================================
; stdio16.inc - input/output routines
;===============================================================================

%ifndef __STDIO16_INC_INCLUDED__
%define __STDIO16_INC_INCLUDED__

[bits 16]

%include "common.inc"

%define     VIDSEG      0xb800
%define     COLS        80                  ; width and height of screen
%define     LINES       25

hex16str    db '0123456789abcdef'
reg16str    db '0000', 0

;-------------------------------------------------------------------------------
; print_char16: prints a character to screen
; bl -> character to print
; bh -> character attribute
;-------------------------------------------------------------------------------

%macro  putc16  2
        push    bx
        mov     bl, %1
        mov     bh, %2
        call    print_char16
        pop     bx
%endmacro

%macro  newline16 0
        putc16  0x0a, 0x00
%endmacro

print_char16:
        pusha
        push    es
        mov     ax, VIDSEG
        mov     es, ax

        ; get current position = x + y * screen_width
        mov     cx, COLS*2                  ; Mode 7 has 2 bytes per char
        movzx   ax, byte [YPOS]           ; YPOS is the current line count
        mul     cx                          ; y * (COLS*2)
        push    ax                          ; ax = y * screen_width

        movzx   ax, byte [XPOS]
        shl     al, 1                       ; ax = XPOS * 2 because it is 2 bytes per char
        pop     cx                          ; cx = y * screen_width
        add     ax, cx                      ; ax = offset address to draw the character at

        ; watch for new line
        cmp     bl, 0x0a                    ; is it a newline charcter ?
        je      .row                        ; if it is, go to next row

        ; print a character
        mov     dx, bx
        mov     bx, ax
        mov     word [es:bx], dx            ; write to video display

        ; update next position
        inc     byte [XPOS]               ; go to the next character
        cmp     byte [XPOS], COLS         ; end of line ?
        je      .row
        jmp     .done

    .row:
        mov     byte [XPOS], 0
        inc     byte [YPOS]

    .done:
        pop     es
        popa
        ret

;-------------------------------------------------------------------------------
; print_string16: prints a null terminated string
; es:si -> address of string to print
;-------------------------------------------------------------------------------

%macro  puts16  2
        push    si
        push    bx
        mov     si, %1
        mov     bh, %2
        call    print_string16
        pop     bx
        pop     si
%endmacro

print_string16:
        pusha

    .loop:
        ; get character
        mov     bl, byte [es:si]            ; get next character
        cmp     bl, 0                       ; null terminator ?
        je      .done

        ; print the character
        call    print_char16

        ; go to the next character
        inc     si
        jmp     .loop

    .done:
        ; update hardware cursor - it's more efficient to update the cursor
        ; after displaying the complete string because direct VGA is slow

        ;mov     bh, byte [YPOS]
        ;mov     bl, byte [XPOS]
        ;call    move_cursor

        popa
        ret

;-------------------------------------------------------------------------------
; print_reg16: prints a 16 bit register
; ax -> register to print
;-------------------------------------------------------------------------------

%macro dumpr16  1
        mov     ax, %1
        call    print_reg16
%endmacro

print_reg16:
        push    di
        push    si
        push    bx
        push    cx
        mov     di, reg16str
        mov     si, hex16str
        mov     cx, 4
    .hexloop:
        rol     ax, 4
        mov     bx, ax
        and     bx, 0x0f
        mov     bl, [si+bx]
        mov     [di], bl
        inc     di
        dec     cx
        jnz     .hexloop
        puts16  reg16str, TEXTATT
        pop     cx
        pop     bx
        pop     si
        pop     di
        ret

%endif ;__STDIO16_INC_INCLUDED__
