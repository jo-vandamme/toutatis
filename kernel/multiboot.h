#ifndef __KERNEL_MULTIBOOT_H__
#define __KERNEL_MULTIBOOT_H__

#include <types.h>

#define MULTIBOOT_MAGIC         0x2badb002
#define MULTIBOOT_MEMINFO       (1 << 0)
#define MULTIBOOT_BOOTDEV       (1 << 1)
#define MULTIBOOT_MODS          (1 << 3)
#define MULTIBOOT_MMAP          (1 << 6)
#define MULTIBOOT_LOADER        (1 << 9)

typedef struct __attribute__((packed))
{
        uint32_t flags;
        uint32_t mem_lower;
        uint32_t mem_upper;
        uint32_t boot_device;
        uint32_t cmd_line;
        uint32_t mods_count;
        uint32_t mods_addr;
        uint32_t num;
        uint32_t size;
        uint32_t addr;
        uint32_t shndx;
        uint32_t mmap_length;
        uint32_t mmap_addr;
        uint32_t drives_length;
        uint32_t drives_addr;
        uint32_t config_table;
        uint32_t boot_loader_name;
        uint32_t apm_table;
        uint32_t vbe_control_info;
        uint32_t vbe_mode_info;
        uint16_t vbe_mode;
        uint16_t vbe_interface_seg;
        uint16_t vbe_interface_off;
        uint16_t vbe_interface_len;
} multiboot_info_t;

typedef struct __attribute__((packed))
{
        uint32_t size;
        uint64_t addr;
        uint64_t length;
        uint32_t type;
} mmap_entry_t;

#endif
