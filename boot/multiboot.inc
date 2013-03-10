;===============================================================================
; Multiboot information structure.
;===============================================================================

%ifndef __MULTIBOOT_INC_INCLUDED__
%define __MULTIBOOT_INC_INCLUDED__

%define MULTIBOOT_MAGIC         0x2badb002
%define MULTIBOOT_MEMINFO       (1 << 0)
%define MULTIBOOT_BOOTDEV       (1 << 1)
%define MULTIBOOT_MMAP          (1 << 6)
%define MULTIBOOT_LOADER        (1 << 9)

struc multiboot_info
        .flags                  resd 1  ; required
        .memory_lower           resd 1  ; present if flags[0] is set
        .memory_upper           resd 1  ; present if flags[0] is set
        .boot_device            resd 1  ; present if flags[1] is set
        .cmd_line               resd 1  ; present if flags[2] is set
        .mods_count             resd 1  ; present if flags[3] is set
        .mods_addr              resd 1  ; present if flags[3] is set
        .syms0                  resd 1  ; symbol table info, present if flags[4] or flags[5] is set
        .syms1                  resd 1  ; present if flags[4] or flags[5] is set
        .syms2                  resd 1  ; present if flags[4] or flags[5] is set
        .syms3                  resd 1  ; present if flags[4] or flags[5] is set
        .mmap_length            resd 1  ; present if flags[6] is set
        .mmap_addr              resd 1  ; present if flags[6] is set
        .drives_length          resd 1  ; present if flags[7] is set
        .drives_addr            resd 1  ; present if flags[7] is set
        .config_table           resd 1  ; ROM configurable table, present if flags[8] is set
        .boot_loader_name       resd 1  ; present if flags[9] is set
        .apm_table              resd 1  ; advanced power management table, present if flags[10] is set
        .vbe_control_info       resd 1  ; video bios extension, present if flags[11] is set
        .vbe_mode_info          resd 1  ; present if flags[11] is set
        .vbe_mode               resw 1  ; present if flags[11] is set
        .vbe_interface_seg      resw 1  ; present if flags[11] is set
        .vbe_interface_off      resw 1  ; present if flags[11] is set
        .vbe_interface_len      resw 1  ; present if flags[11] is set
endstruc

%endif ;__MULTIBOOT_INC_INCLUDED__
