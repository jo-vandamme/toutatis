;===============================================================================
; stdio32.inc - input/output routines
;===============================================================================

%ifndef __STDIO32_INC_INCLUDED__
%define __STDIO32_INC_INCLUDED__

[bits 32]

%include "common.inc"

%define     VIDMEM      0xb8000             ; video memory
%define     COLS        80                  ; width and height of screen
%define     LINES       25

hex32str db '0123456789abcdef'
reg32str db '00000000', 0

;-------------------------------------------------------------------------------
; print_char32: prints a character to screen
; bl -> character to print
; bh -> character attribute
;-------------------------------------------------------------------------------

%macro  putc32  2
        push    bx
        mov     bl, %1
        mov     bh, %2
        call    print_char32
        pop     bx
%endmacro

%macro  newline32 0
        putc32  0x0a, 0x00
%endmacro

print_char32:
        pusha
        mov     edi, VIDMEM                 ; get pointer to video memory

        ; get current position = x + y * screen_width
        mov     ecx, COLS*2                 ; Mode 7 has 2 bytes per char
        movzx   eax, byte [YPOS]            ; YPOS is the current line count
        mul     ecx                         ; y * (COLS*2)
        push    eax                         ; eax = y * screen_width

        movzx   eax, byte [XPOS]
        shl     eax, 1                      ; eax = XPOS * 2 because it is 2 bytes per char
        pop     ecx                         ; ecx = y * screen_width
        add     eax, ecx                    ; eax = offset address to draw the character at

        add     edi, eax                    ; add eax to the base address

        ; watch for new line
        cmp     bl, 0x0a                    ; is it a newline charcter ?
        je      .row                        ; if it is, go to next row

        ; print a character
        mov     word [edi], bx              ; write to video display

        ; update next position
        inc     byte [XPOS]               ; go to the next character
        cmp     byte [XPOS], COLS         ; end of line ?
        je      .row
        jmp     .done

    .row:
        mov     byte [XPOS], 0
        inc     byte [YPOS]

    .done:
        popa
        ret

;-------------------------------------------------------------------------------
; print_string32: prints a null terminated string
; esi -> address of string to print
;-------------------------------------------------------------------------------

%macro  puts32  2
        push    esi
        push    bx
        mov     esi, %1
        mov     bh, %2
        call    print_string32
        pop     bx
        pop     esi
%endmacro

print_string32:
        pusha

    .loop:
        ; get character
        mov     bl, byte [esi]              ; get next character
        cmp     bl, 0                       ; null terminator ?
        je      .done

        ; print the character
        call    print_char32

        ; go to the next character
        inc     esi
        jmp     .loop

    .done:
        ; update hardware cursor - it's more efficient to update the cursor
        ; after displaying the complete string because direct VGA is slow

        mov     bh, byte [YPOS]
        mov     bl, byte [XPOS]
        call    move_cursor

        popa
        ret

;-------------------------------------------------------------------------------
; print_reg32: prints a 32 bit register
; eax -> register to print
;-------------------------------------------------------------------------------

%macro dumpr32  1
        push    eax
        mov     eax, %1
        call    print_reg32
        pop     eax
%endmacro

print_reg32:
        push    edi
        push    esi
        push    ebx
        push    ecx
        mov     edi, reg32str
        mov     esi, hex32str
        mov     ecx, 4
    .hexloop:
        rol     eax, 4
        mov     ebx, eax
        and     ebx, 0x0000000f
        mov     bl, [si+bx]
        mov     [edi], bl
        inc     edi
        dec     ecx
        jnz     .hexloop
        puts32  reg32str, TEXTATT
        pop     ecx
        pop     ebx
        pop     esi
        pop     edi
        ret

;-------------------------------------------------------------------------------
; moveCur: update hardware cursor
; bh -> y position
; bl -> x position
;-------------------------------------------------------------------------------

move_cursor:
        pusha

        ; get current position - location = XPOS + YPOS * cols

        xor     eax, eax
        mov     ecx, COLS
        mov     al, bh                      ; get y position
        mul     ecx                         ; eax = y * COLS
        add     al, bl                      ; eax = y * COLS + x
        mov     ebx, eax                    ; ebx = y * COLS + x

        ; set low byte index to VGA register
        xor     eax, eax
        mov     al, 0x0f
        mov     dx, 0x03d4
        out     dx, al

        mov     al, bl
        mov     dx, 0x03d5
        out     dx, al                      ; low byte

        ; set high byte index to VGA register
        xor     eax, eax
        mov     al, 0x0e
        mov     dx, 0x03d4
        out     dx, al

        mov     al, bh
        mov     dx, 0x03d5
        out     dx, al                      ; high byte

        popa
        ret

;-------------------------------------------------------------------------------
; clear_screen_32: clears screen
; ah -> character attribute
;-------------------------------------------------------------------------------

clear_screen_32:
        pusha
        cld

        mov     edi, VIDMEM
        mov     cx, COLS * LINES
        mov     al, ' '
        rep     stosw

        mov     byte [XPOS], 0
        mov     byte [YPOS], 0
        mov     bh, 0
        mov     bl, 0
        call    move_cursor

        popa
        ret

;-------------------------------------------------------------------------------
; goto_xy: set current x/y location
; al -> x position
; ah -> y position
;-------------------------------------------------------------------------------

goto_xy:
        pusha
        mov     [XPOS], al
        mov     [YPOS], ah
        mov     bh, ah
        mov     bl, al
        call    move_cursor
        popa
        ret

%endif ;__STDIO32_INC_INCLUDED__
