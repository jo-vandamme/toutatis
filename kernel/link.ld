OUTPUT_FORMAT("elf32-i386")
ENTRY("bootstrap")

kernel_phys    = 0x00100000;
kernel_voffset = 0xc0000000;

SECTIONS
{
        . = kernel_phys + kernel_voffset;

        kernel_start = .;

        /* Actual code */
        .text : AT(kernel_phys) {
                kernel_code = .;
                *(.text)
                *(.rodata*)
                . = ALIGN(0x1000);
        }

        /* Kernel data */
        .data : AT(kernel_phys + (kernel_data - kernel_code)) {
                kernel_data = .;
                *(.data)
                . = ALIGN(0x1000);
        }

        /* Statically define, uninitialized values */
        .bss : AT(kernel_phys + (kernel_bss - kernel_code)) {
                kernel_bss = .;
                *(.COMMON)
                *(.bss)
                . = ALIGN(0x1000);
        }

        /* End of kernel */
        kernel_end = .;

        /* Get rid of unessecary GCC bits */
        /DISCARD/ : {
                *(.comment)
                *(.eh_frame)
                *(.note.gnu.build-id)
                *(.debug_abbrev)
                *(.debug_info)
                *(.debug_line)
                *(.debug_loc)
                *(.debug_pubnames)
                *(.debug_aranges)
                *(.debug_str)
                *(.debug_macinfo)
                *(.debug_ranges)
                *(.stab)
                *(.stabstr)
        }
}
