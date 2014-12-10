#ifndef __KERNEL_VIRT_MEM_H__
#define __KERNEL_VIRT_MEM_H__

#include <types.h>

#define PAGE_DIRECTORY_INDEX(virt) (((virt) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(virt)     (((virt) >> 12) & 0x3ff)
#define PAGE_FRAME_ADDRESS(virt)   ((virt) & 0xfff);

typedef u32_t virtual_addr_t;

/* page table entry */
typedef struct
{
    u32_t present           : 1;
    u32_t read_write        : 1;
    u32_t user_supervisor   : 1;
    u32_t write_through     : 1;
    u32_t cache_disabled    : 1;
    u32_t accessed          : 1;
    u32_t dirty             : 1;
    u32_t pt_attr_index     : 1;
    u32_t global_page       : 1;
    u32_t available         : 3;
    u32_t frame_address     : 20;
} pte_t;

/* page directory entry */
typedef struct
{
    u32_t present           : 1;
    u32_t read_write        : 1;
    u32_t user_supervisor   : 1;
    u32_t write_through     : 1;
    u32_t cache_disabled    : 1;
    u32_t accessed          : 1;
    u32_t reserved          : 1;
    u32_t page_size         : 1;
    u32_t global_page       : 1;
    u32_t available         : 3;
    u32_t pt_base_address   : 20;
} pde_t;

/* page table */
typedef struct
{
    pte_t entries[1024];
} page_table_t;

/* page directory */
typedef struct
{
    pde_t entries[1024];
} page_dir_t;

void vmm_init();
int vmm_switch_directory(page_dir_t *dir);

#endif
