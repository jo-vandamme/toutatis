#ifndef __KERNEL_PAGING_H__
#define __KERNEL_PAGING_H__

#include <system.h>
#include <types.h>

#define FRAME_SIZE 0x1000

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
    uint32_t frame             : 20;
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
    uint32_t page_table_base   : 20;
} pde_t;

/* page table */
typedef struct
{
    pte_t pages[1024];
} page_table_t;

/* page directory */
typedef struct page_dir
{
    pde_t entries[1024]; /* this must be the first field of the struct */
    uintptr_t entries_phys_addr; /* physical address of the first field */

    page_table_t *tables[1024]; /* array of pointers to page tables */
} page_dir_t;

int test_frame(uint32_t frame);
void set_frame(uint32_t frame);
void clear_frame(uint32_t frame);
int32_t first_free_frames(size_t num);
int32_t first_free_frame();

uint32_t memory_used();
uint32_t memory_total();

void alloc_page(pte_t *page, int is_kernel, int is_writeable);
void map_page(pte_t *page, int is_kernel, int is_writeable, uintptr_t phys);
void free_page(pte_t *page);
pte_t *get_page(uintptr_t virt, int make, page_dir_t *dir);
void paging_init();
void paging_finalize();
void paging_mark_reserved(uintptr_t address);

page_dir_t *switch_page_directory(page_dir_t *dir);
void invalidate_page_tables_at(uintptr_t addr);
page_dir_t *clone_page_directory(page_dir_t *dir);

void page_fault(registers_t *regs);

#endif
