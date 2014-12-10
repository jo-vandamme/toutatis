#include <logging.h>
#include <string.h>
#include <cpu.h>
#include "virt_mem.h"
#include "phys_mem.h"

extern u32_t kernel_voffset;
extern u32_t kernel_start;

page_dir_t *current_dir = 0;
phys_addr_t current_pdbr = 0;

void vmm_init()
{
    /* allocate default page table */
    page_table_t *table = (page_table_t *)pmm_alloc_frame();
    if (!table)
        return;

    /* allocate 3gb page table */
    page_table_t *table2 = (page_table_t *)pmm_alloc_frame();
    if (!table2)
        return;

    /* clear page table */
    memset(table, 0, sizeof(page_table_t));

    /* 1st 4MB are identity mapped */
    for (u32_t i = 0, frame = 0, virt = 0;
         i < 1024;
         ++i, frame += FRAME_SIZE, virt += FRAME_SIZE)
    {
        /* create a new page */
        pte_t page = { 0 };
        memset(&page, 0, sizeof(pte_t));
        page.present = 1;
        page.frame_address = frame;

        table2->entries[PAGE_TABLE_INDEX(virt)] = page;
    }

    /* map 0MB to 3GB */
    for (u32_t i = 0, frame = 0, virt = (u32_t)&kernel_voffset;
         i < 1024;
         ++i, frame += FRAME_SIZE, virt += FRAME_SIZE)
    {
        pte_t page = { 0 };
        memset(&page, 0, sizeof(pte_t));
        page.present = 1;
        page.frame_address = frame >> 12;

        table->entries[PAGE_TABLE_INDEX(virt)] = page;
    }

    /* create default directory table */
    page_dir_t *dir = (page_dir_t *)pmm_alloc_frames(3);
    if (!dir)
        return;

    /* clear directory table and set it as current */
    memset(dir, 0, sizeof(page_dir_t));

    /* get first entry in dir table and set it up to point to our table */
    pde_t *entry = &dir->entries[PAGE_DIRECTORY_INDEX((u32_t)&kernel_start)];
    entry->present = 1;
    entry->read_write = 1;
    entry->pt_base_address = ((phys_addr_t)table) >> 12;

    pde_t *entry2 = &dir->entries[PAGE_DIRECTORY_INDEX(0)];
    entry2->present = 1;
    entry2->read_write = 1;
    entry2->pt_base_address = ((phys_addr_t)table2) >> 12;

    /* store current PDBR */
    current_pdbr = (phys_addr_t)&dir->entries;

    /* switch to our page directory */
    vmm_switch_directory(dir);

    /* enable paging */
    cpu_enable_paging(1);
}

int vmm_switch_directory(page_dir_t *dir)
{
    if (!dir)
        return 0;

    current_dir = dir;
    cpu_set_pdbr(current_pdbr);
    return 1;
}
