;===============================================================================
; common.inc - various defintions to access the Bios Parameter Block, the FAT
; and the root directory in memory
;===============================================================================

%ifndef __COMMON_INC_INCLUDED__
%define __COMMON_INC_INCLUDED__

[bits 16]

%define BASE            0x7c00                  ; the BIOS loads the boot sector into memory location 0x7C00
%define FATBUF          0x7e00
%define ROOTDIR         0xae00

%define oemName         BASE + 0x03             ; OEM label
%define bytesPerSect    BASE + 0x0b             ; bytes/sector
%define sectPerClust    BASE + 0x0d             ; sectors/allocation unit
%define nResSectors     BASE + 0x0e             ; # reserved sectors
%define nFATs           BASE + 0x10             ; # of fats
%define nRootDirEnts    BASE + 0x11             ; # of root dir entries
%define nSectors        BASE + 0x13             ; # sectors total in image
%define media           BASE + 0x15             ; media descrip: fd=2side9sec, etc...
%define sectPerFAT      BASE + 0x16             ; # sectors in a fat
%define sectPerTrack    BASE + 0x18             ; # sectors/track
%define nHeads          BASE + 0x1a             ; # heads
%define nHidden         BASE + 0x1c             ; # hidden sectors
%define nSectorHuge     BASE + 0x20             ; # sectors if > 65536
%define drive           BASE + 0x24             ; drive number
%define unused          BASE + 0x25             ; reserved
%define extBoot         BASE + 0x26             ; extended boot signature
%define volId           BASE + 0x27             ; volume serial number
%define volLabel        BASE + 0x2b             ; volume label (11 characters)
%define fileSys         BASE + 0x36             ; filesystem id (8 characters)

%define STAGE2_SEG      0x0050
%define STAGE2_OFF      0x0000

%define XPOS            0xde00                  ; current character x position
%define YPOS            0xde01                  ; current character y position
%define DATA_SECTOR     0xde02                  ; first data sector
%define KERNEL_SIZE     0xde04                  ; kernel size in bytes

; The kernel is loaded at 64KB in real mode
%define KERNEL_RSEG     0x1000                  ; kernel real mode segment
%define KERNEL_ROFF     0x0000                  ; kernel real mode offset
%define KERNEL_RBASE    KERNEL_RSEG * 16 + KERNEL_ROFF ; kernel real mode address

; The kernel is then moved to 1MB in protected mode
%define KERNEL_PBASE    0x100000                ; kernel protected mode address

%define MMAP_SEG        0x0000
%define MMAP_OFF        0x1000

%define BOOTLOADER_NAME "Toutatis Bootloader"
%define STAGE2_NAME     "LOADER  BIN"
%define KERNEL_NAME     "KERNEL  ELF"

%define TEXTATT         0x08
%define S2ATT           0x0e
%define S3ATT           0x02
%define S4ATT           0x0b
%define ERRATT          0x0c

%endif ;__COMMON_INC_INCLUDED__
