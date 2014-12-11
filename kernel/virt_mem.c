#include <logging.h>
#include <string.h>
#include <cpu.h>
#include "virt_mem.h"
#include "phys_mem.h"

extern u32_t kernel_voffset;
extern u32_t kernel_start;

page_dir_t *current_dir = 0;

void vmm_init()
{
    /* create default directory table */
    page_dir_t *dir = (page_dir_t *)pmm_alloc_frame();
    if (!dir)
        return;

    /* allocate default page table */
    page_table_t *table_ker = (page_table_t *)pmm_alloc_frame();
    if (!table_ker)
        return;

    /* clear page directory and page tables */
    memset(dir, 0, sizeof(page_dir_t));
    memset(table_ker, 0, sizeof(page_table_t));

    /* map [0MB-4MB] to [3GB-3GB+4MB] */
    for (u32_t i = 0, frame = 0, virt = (u32_t)&kernel_voffset;
         i < 1024;
         ++i, frame += FRAME_SIZE, virt += FRAME_SIZE)
    {
        pte_t page = { 0 };
        page.present = 1;
        page.read_write = 0; /* kernel code not writeable from userspace */
        page.user_supervisor = 0;
        page.frame_address = frame >> 12;

        table_ker->entries[PAGE_TABLE_INDEX(virt)] = page;
    }

    /* set entries in directory to point to kernel table */
    pde_t *entry_ker = &dir->entries[PAGE_DIRECTORY_INDEX((u32_t)&kernel_start)];
    entry_ker->present = 1;
    entry_ker->read_write = 0;
    entry_ker->user_supervisor = 0;
    entry_ker->pt_base_address = ((phys_addr_t)table_ker) >> 12;

    /* before we enable paging, we must register the page fault handler */
    attach_interrupt_handler(14, page_fault);

    /* switch to our page directory */
    vmm_set_page_directory(dir);
}

inline int vmm_set_page_directory(page_dir_t *dir)
{
    if (!dir)
        return 0;

    current_dir = dir;
    cpu_set_pdbr((phys_addr_t)&dir->entries);

    /* enable paging */
    cpu_enable_paging(1);

    return 1;
}

inline page_dir_t *vmm_get_page_directory()
{
    return current_dir;
}

int vmm_alloc_page(pte_t *page, int is_kernel, int is_writeable)
{
    /* if the frame was already allocated, return straight away */
    if (page->frame_address != 0) {
        return 0;
    }

    /* allocate a free physical frame */
    phys_addr_t frame = pmm_alloc_frame();
    if (!frame) {
        return 0;
    }

    /* map the frame to the page */
    page->frame_address = frame >> 12;
    page->present = 1;
    page->read_write = (is_writeable) ? 1 : 0;
    page->user_supervisor = (is_kernel) ? 0 : 1;

    return 1;    
}

void vmm_free_page(pte_t *page)
{
    phys_addr_t frame = page->frame_address << 12;
    if (frame) {
        pmm_free_frame(frame);
    }

    page->present = 0;
    page->frame_address = 0x0;
}

void vmm_map_page(phys_addr_t phys, virt_addr_t virt)
{
    /* get page directory */
    page_dir_t *page_dir = vmm_get_page_directory();

    /* get page table */
    pde_t *entry = &page_dir->entries[PAGE_DIRECTORY_INDEX(virt)];

    page_table_t *table;
    if (!entry->present)
    {
        /* page table not present, allocate it */
        table = (page_table_t *)pmm_alloc_frame();
        if (!table) {
            return;
        }

        /* clear page table */
        memset(table, 0, sizeof(page_table_t));

        /* map the page table */
        entry->present = 1;
        entry->read_write = 1;
        entry->pt_base_address = ((phys_addr_t)table) >> 12;
    }

    /* get the page table */
    table = (page_table_t *)(entry->pt_base_address << 12);

    /* get the page */
    pte_t *page = &table->entries[PAGE_TABLE_INDEX(virt)];

    /* map the page */
    page->frame_address = phys >> 12;
    page->present = 1;
}

void page_fault(registers_t *regs)
{
    kprintf(INFO, "hello %#010x\n", regs->eip);
    halt();
}
