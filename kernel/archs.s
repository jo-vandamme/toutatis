[global halt]
halt:
    hlt
    ret

[global sti]
sti:
    sti
    ret

[global cli]
cli:
    cli
    ret

[global io_wait]
io_wait:
    in  al, 0x80
    out 0x80, al                ; port 0x80 is used for 'checkpoints during POST

[global interrupt]
interrupt:
    mov al, byte [esp + 4]          ; interrupt number
    mov byte [.interrupt + 1], al   ; this overwrites the 0 below
    jmp .interrupt
    .interrupt:
        int 0
        ret

[global outb]
outb:
    mov dx, word [esp + 4]      ; port number
    mov al, byte [esp + 8]      ; byte to write
    out dx, al
    ret

[global outw]
outw:
    mov dx, word [esp + 4]      ; port number
    mov ax, word [esp + 8]      ; word to write
    out dx, ax
    ret

[global outl]
outl:
    mov dx,  word  [esp + 4]    ; port number
    mov eax, dword [esp + 8]    ; double word to write
    out dx, eax
    ret

[global inb]
inb:
    mov dx, word [esp + 4]      ; port number
    in  al, dx                  ; store the value read in al
    ret

[global inw]
inw:
    mov dx, word [esp + 4]      ; port number
    in  ax, dx                  ; store the value read in ax
    ret

[global inl]
inl:
    mov dx, word [esp + 4]      ; port number
    in  eax, dx                 ; store the value read in eax
    ret

