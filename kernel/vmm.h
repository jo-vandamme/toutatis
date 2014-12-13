#ifndef __KERNEL_VIRT_MEM_H__
#define __KERNEL_VIRT_MEM_H__

#include <system.h>
#include <types.h>
#include <pmm.h>

#define PAGE_DIRECTORY_INDEX(virt) (((virt) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(virt)     (((virt) >> 12) & 0x3ff)
#define PAGE_FRAME_ADDRESS(virt)   ((virt) & 0xfff);

typedef uintptr_t virt_addr_t;

/* page table entry */
typedef struct
{
    uint32_t present           : 1;
    uint32_t read_write        : 1;
    uint32_t user_supervisor   : 1;
    uint32_t write_through     : 1;
    uint32_t cache_disabled    : 1;
    uint32_t accessed          : 1;
    uint32_t dirty             : 1;
    uint32_t pt_attr_index     : 1;
    uint32_t global_page       : 1;
    uint32_t available         : 3;
    uint32_t frame_address     : 20;
} pte_t;

/* page directory entry */
typedef struct
{
    uint32_t present           : 1;
    uint32_t read_write        : 1;
    uint32_t user_supervisor   : 1;
    uint32_t write_through     : 1;
    uint32_t cache_disabled    : 1;
    uint32_t accessed          : 1;
    uint32_t reserved          : 1;
    uint32_t page_size         : 1;
    uint32_t global_page       : 1;
    uint32_t available         : 3;
    uint32_t pt_base_address   : 20;
} pde_t;

/* page table */
typedef struct
{
    pte_t pages[1024];
} page_table_t;

/* page directory */
typedef struct
{
    pde_t tables[1024];
} page_dir_t;

void vmm_init();

int vmm_switch_page_directory(page_dir_t *dir);
void invalidate_page_tables_at(uintptr_t addr);

pte_t *vmm_get_page(virt_addr_t virt, int make, page_dir_t *dir);

int vmm_alloc_page(pte_t *page, int is_kernel, int is_writeable);
void vmm_free_page(pte_t *page);

void page_fault(registers_t *regs);

#endif
