;===============================================================================
; Enable A20 address line so we correctly address memory above 1MB.
;===============================================================================

%ifndef __A20_INC_INCLUDED__
%define __A20_INC_INCLUDED__

[bits 16]

;-------------------------------------------------------------------------------
; Enable A20 line through output port
;-------------------------------------------------------------------------------

enableA20:

        cli
        pusha

        call    .wait_input
        mov     al, 0xad
        out     0x64, al                ; disable keyboard

        call    .wait_input
        mov     al, 0xd0
        out     0x64, al                ; tell controller to read output port

        call    .wait_output
        in      al, 0x60
        push    eax                     ; get output port data and store it

        call    .wait_input
        mov     al, 0xd1
        out     0x64, al                ; tell controller to write output port

        call    .wait_input
        pop     eax
        or      al, 2                   ; set second bit (enable a20)
        out     0x60, al                ; write out data back to the output port

        call    .wait_input
        mov     al, 0xae                ; enable keyboard
        out     0x64, al

        call    .wait_input
        popa
        sti
        ret

    .wait_input:

        ; wait for input buffer to be clear to make sure the controller is done
        ; processing commands
        in      al, 0x64
        test    al, 2                   ; second bit for input buffer status
        jnz     .wait_input             ; 0 = buffer empty -> write to it
        ret

    .wait_output:

        ; wait for output buffer to be full
        in      al, 0x64
        test    al, 1                   ; first bit for output buffer status
        jz      .wait_output            ; 1 = buffer full -> read from it
        ret

%endif ;__A20_INC_INCLUDED__
