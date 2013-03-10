#ifndef __KERNEL_MULTIBOOT_H__
#define __KERNEL_MULTIBOOT_H__

#define MULTIBOOT_MAGIC         0x2badb002
#define MULTIBOOT_MEMINFO       (1 << 0)
#define MULTIBOOT_BOOTDEV       (1 << 1)
#define MULTIBOOT_MMAP          (1 << 6)
#define MULTIBOOT_LOADER        (1 << 9)

typedef struct __attribute__((packed))
{
        u32_t flags;
        u32_t mem_lower;
        u32_t mem_upper;
        u32_t boot_device;
        u32_t cmd_line;
        u32_t mods_count;
        u32_t mods_addr;
        u32_t num;
        u32_t size;
        u32_t addr;
        u32_t shndx;
        u32_t mmap_length;
        u32_t mmap_addr;
        u32_t drives_length;
        u32_t drives_addr;
        u32_t config_table;
        u32_t boot_loader_name;
        u32_t apm_table;
        u32_t vbe_control_info;
        u32_t vbe_mode_info;
        u16_t vbe_mode;
        u16_t vbe_interface_seg;
        u16_t vbe_interface_off;
        u16_t vbe_interface_len;
} multiboot_info_t;

typedef struct __attribute__((packed))
{
        u32_t size;
        u32_t addr_low;
        u32_t addr_high;
        u32_t length_low;
        u32_t length_high;
        u32_t type;
} mmap_entry_t;

#endif
