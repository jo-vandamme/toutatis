#ifndef __KERNEL_VIRT_MEM_H__
#define __KERNEL_VIRT_MEM_H__

#include <types.h>
#include <phys_mem.h>
#include <arch.h>

#define PAGE_DIRECTORY_INDEX(virt) (((virt) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(virt)     (((virt) >> 12) & 0x3ff)
#define PAGE_FRAME_ADDRESS(virt)   ((virt) & 0xfff);

typedef u32_t virt_addr_t;

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

int vmm_set_page_directory(page_dir_t *dir);
page_dir_t *vmm_get_page_directory();

int vmm_alloc_page(pte_t *page, int is_kernel, int is_writeable);
void vmm_free_page(pte_t *page);

void vmm_map_page(phys_addr_t phys, virt_addr_t virt);

void page_fault(registers_t *regs);

#endif
