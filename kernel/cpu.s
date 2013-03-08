[global cpu_get_gdt]
cpu_get_gdt:
        sgdt    [eax]
        ret

[global cpu_get_idt]
cpu_get_idt:
        sidt    [eax]
        ret

[global cpu_set_gdt]
cpu_set_gdt:
        mov     eax, [esp + 4]  ; get gdt pointer
        lgdt    [eax]           ; load new gdt pointer
        jmp     0x08:.reload_cs ; far jump to load the kernel code descriptor in cs
    .reload_cs:
        mov     ax, 0x10        ; set all the segment selectors to kernel data descriptor
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        mov     ss, ax
        ret

[global cpu_set_idt]
cpu_set_idt:
        mov     eax, [esp + 4]  ; get idt pointer
        lidt    [eax]           ; load new idt pointer
        ret
