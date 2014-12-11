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

[global cpu_set_pdbr]
cpu_set_pdbr:
        mov     eax, [esp + 4] ; get addr
        mov     cr3, eax
        ret

[global cpu_get_pdbr]
cpu_get_pdbr:
        mov     eax, cr3
        ret

[global cpu_enable_paging]
cpu_enable_paging:
        mov     eax, cr0
        mov     ebx, [esp + 4] ; get flag
        cmp     ebx, 1
        je      enable
        jmp     disable
enable:
        or      eax, 0x80000000 ; set bit 31
        mov     cr0, eax
        jmp     done
disable:
        and     eax, 0x7fffffff ; clear bit 31
        mov     cr0, eax
done:
        ret

[global cpu_flush_tlb_entry]
cpu_flush_tlb_entry:
        mov     eax, [esp + 4]
        cli
        invlpg  [eax]
        sti
