;===============================================================================
; memory.inc
;===============================================================================

%ifndef __MEMORY_INC_INCLUDED__
%define __MEMORY_INC_INCLUDED__

;-------------------------------------------------------------------------------
; get_low_memory_size: number of KB available below 1MB
; ax <- KB between 0 and 1MB, ax = -1 on error
;-------------------------------------------------------------------------------

get_low_memory_size:
        xor     ax, ax
        int     0x12
        jc      .error
        test    ax, ax
        jz      .error
        jmp     .done
    .error:
        mov     ax, -1
    .done:
        ret


;-------------------------------------------------------------------------------
; get_hith_memory_size: get memory size for configurations with more than 64MB
; ax <- KB between 1MB and 16MB
; bx <- number of 64KB blocks above 16MB
; bx = 0 and ax = -1 on error
;-------------------------------------------------------------------------------

get_high_memory_size:
        push    ecx
        push    edx
        xor     ecx, ecx
        xor     edx, edx
        mov     ax, 0xe801
        int     0x15
        jc      short .error
        cmp     ah, 0x86        ; unsupported function
        je      short .error
        cmp     ah, 0x80        ; invalid command
        je      short .error
        jcxz    .use_ax         ; bios may store the result in ax,bx or cx,dx
        mov     ax, cx
        mov     bx, dx
    .use_ax:
        pop     edx
        pop     ecx
        ret
    .error:
        mov     ax, -1
        mov     bx, 0
        pop     edx
        pop     ecx
        ret

;-------------------------------------------------------------------------------
; get_memory_map: get memory map from BIOS
; es:di -> destination buffer for entries
; bp    <- number of entries
; carry = 1 upon error
;-------------------------------------------------------------------------------

struc memory_map_entry
        .base_address   resq 1
        .length         resq 1
        .type           resd 1
        .acpi_null      resd 1
endstruc

get_memory_map:
        push    eax
        push    ebx
        push    ecx
        push    edx
        xor     bp, bp          ; number of entries will be stored in bp
        mov     eax, 0xe820
        xor     ebx, ebx
        mov     ecx, 24         ; memory map entry struct is 24 bytes
        mov     edx, 'PAMS'     ; 'SMAP'
        int     0x15            ; get first entry
        jc      .error
        test    ebx, ebx        ; if ebx = 0 then list is one entry long, so bail out
        je      .error
        jmp     .test_entry
    .good_entry:
        inc     bp              ; increment entry count
        add     di, 24          ; next entry in buffer
    .next_entry:
        cmp     ebx, 0          ; if ebx is 0, then we are done
        je      .done
        mov     eax, 0xe820
        mov     ecx, 24
        mov     edx, 'PAMS'
        int     0x15
    .test_entry:
        jcxz    .next_entry     ; if actual length returned in bytes is 0, skip entry
        mov     ecx, [es:di + memory_map_entry.length]  ; lower 4 bytes of length
        test    ecx, ecx
        jne     short .good_entry ; if length is not 0, this is a valid entry
        mov     ecx, [es:di + memory_map_entry.length + 4] ; upper 4 bytes of length
        jecxz   .next_entry     ; if length is 0 skip entry
    .error:
        stc
        jmp     .return
    .done:
        clc
    .return:
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        ret

%endif ;__MEMORY_INC_INCLUDED__

