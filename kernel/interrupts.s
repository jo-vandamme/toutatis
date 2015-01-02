;===============================================================================
; interrupts.s - interrupt service routine wrappers
;===============================================================================

; This macro creates a stub for an ISR which does not
; pass its own error code (adds a dummy errcode byte).
%macro ISR_NOERRCODE 1
    [global isr%1]
    isr%1:
        cli                     ; disable interrupts
        push    dword 0         ; push dummy error code
        push    dword %1        ; push interrupt number
        jmp     isr_common_stub ; jump to common handler
%endmacro

; This macro creates a stub for an ISR which passes its
; own error code.
%macro ISR_ERRCODE 1
    [global isr%1]
    isr%1:
        cli
        push    dword %1        ; push interrupt number
        jmp     isr_common_stub ; jump to common handler
%endmacro

; This macro creates a stub for an IRQ.
%macro IRQ 1
    [global irq%1]
    irq%1:
        cli
        push    dword 0
        push    dword %1
        jmp     irq_common_stub
%endmacro

; ISR 15 is unassigned, 20-31 are reserved
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31
ISR_NOERRCODE 127
IRQ  0
IRQ  1
IRQ  2
IRQ  3
IRQ  4
IRQ  5
IRQ  6
IRQ  7
IRQ  8
IRQ  9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

; This is our common interrupt stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame

%macro INT_HANDLER_STUB 1

[extern %1_handler]
%1_common_stub:         ; the processor already pused ss, esp, eflags, cs, eip
    push    eax
    push    ebx
    push    ecx
    push    edx
    push    esi
    push    edi
    push    ebp
    push    ds
    push    es
    push    fs
    push    gs

    mov     ax, 0x10    ; load the kernel data segment descriptor
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    push    esp         ; esp is pointing below this push (4 bytes above top)
    call    %1_handler
    sub     eax, 4      ; eax contains esp, remove 4 to point to the stack's top
    mov     esp, eax    ; load esp, could have been changed by handler
    pop     eax         ; pop old esp

    pop     gs          ; reload the original data segment descriptor
    pop     fs
    pop     es
    pop     ds
    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax

    add     esp, 8      ; cleans up the stack (error code and isr number)
    iretd               ; pops eip, cs, eflags, esp, ss
    ; iretd will restore eflags and thus re-enable interrupts if needed

%endmacro

INT_HANDLER_STUB isr
INT_HANDLER_STUB irq
