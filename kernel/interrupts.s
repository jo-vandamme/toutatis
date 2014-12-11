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
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 128
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

; This is our common ISR stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame

[extern isr_handler]
isr_common_stub:
    ; the interrupt pushes ss, esp, eflags, cs, eip onto the stack
    ; then it pushes the error code and our isr pushes the interrupt number
    pusha               ; pushes edi, esi, ebp, esp, ebx, edx, ecx, eax
    push    ds
    push    es
    push    fs
    push    gs

    mov     ax, 0x10    ; load the kernel data segment descriptor
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    
    push    esp
    call    isr_handler
    pop     eax

    pop     gs          ; reload the original data segment descriptor
    pop     fs
    pop     es
    pop     ds
    popa

    add     esp, 8      ; cleans up the pushed error code and pushed ISR number
    sti                 ; enable interrupts
    iret                ; pops cs, eip, eflags, ss, esp

; This is our common IRQ stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame

[extern irq_handler]
irq_common_stub:
    pusha
    push    ds
    push    es
    push    fs
    push    gs

    mov     ax, 0x10    ; load the kernel data segment descriptor
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    push    esp
    call    irq_handler
    pop     eax

    pop     gs          ; reload the original data segment descriptor
    pop     fs
    pop     es
    pop     ds
    popa

    add     esp, 8      ; cleans up the stack (error code and isr number)
    sti
    iret                ; pops cs, eip, eflags, ss, esp

