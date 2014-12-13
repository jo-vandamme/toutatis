#include <string.h>
#include <vga.h>
#include "virt_mem.h"
#include "phys_mem.h"

extern uint32_t kernel_voffset;
extern uint32_t kernel_start;

page_dir_t *current_dir = 0;

void vmm_init()
{
    /* create default directory table */
    page_dir_t *dir = (page_dir_t *)pmm_alloc_frame();
    if (!dir) 
        return;

    /* clear page directory and page tables */
    memset(dir, 0, sizeof(page_dir_t));

    /* map [0MB-4MB] to [3GB-3GB+4MB] */
    for (uint32_t i = 0, frame = 0, virt = (uint32_t)&kernel_voffset;
         i < 1024;
         ++i, frame += FRAME_SIZE, virt += FRAME_SIZE)
    {
        pte_t *page = vmm_get_page(virt, 1, dir);
        page->present = 1;
        page->read_write = 0; /* kernel code not writeable from userspace */
        page->user_supervisor = 0;
        page->frame_address = frame >> 12;
        set_frame(frame / FRAME_SIZE);
    }

    /* before we enable paging, we must register the page fault handler */
    attach_interrupt_handler(14, page_fault);

    /* switch to our page directory */
    vmm_switch_page_directory(dir);
}

int vmm_switch_page_directory(page_dir_t *dir)
{
    if (!dir)
        return 0;
    current_dir = dir;

    /* set page directory base register (PDBR) */
    __asm__ volatile("mov %0, %%cr3" :: "r"((phys_addr_t)&dir->tables));

    /* enable paging */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; /* set bit 31 */
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

    return 1;
}

int vmm_alloc_page(pte_t *page, int is_kernel, int is_writeable)
{
    /* if the frame was alread allocated, return straight away */
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

pte_t *vmm_get_page(virt_addr_t virt, int make, page_dir_t *dir)
{
    /* get page table entry */
    pde_t *entry = &dir->tables[PAGE_DIRECTORY_INDEX(virt)];

    /* get the page table and create it first if necessary */
    page_table_t *table;
    if (entry->present)
    {
        table = (page_table_t *)(entry->pt_base_address << 12);
    }
    else if (make)
    {
        /* page table not present, allocate it */
        table = (page_table_t *)pmm_alloc_frame();
        if (!table) {
            return 0;
        }

        /* clear page table */
        memset(table, 0, sizeof(page_table_t));

        /* map the page table */
        entry->present = 1;
        entry->read_write = 1;
        entry->pt_base_address = ((phys_addr_t)table) >> 12;
    }
    else
    {
        return 0;
    }

    return &table->pages[PAGE_TABLE_INDEX(virt)];
}

void page_fault(registers_t *regs)
{
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    int present  = !(regs->err_code & (1 << 0)); // page not present
    int rw       = regs->err_code & (1 << 1);    // write operation
    int us       = regs->err_code & (1 << 2);    // processor was in user-mode
    int reserved = regs->err_code & (1 << 3);    // overwritten cpu-reserved bits of pte
    int id       = regs->err_code & (1 << 4);    // occured during an instruction fetch

    vga_print_str("\033\014Page fault! (");
    if (present)  { vga_print_str("not present "); }
    if (rw)       { vga_print_str("read-only "); }
    if (us)       { vga_print_str("user-mode "); }
    if (reserved) { vga_print_str("reserved "); }
    if (id)       { vga_print_str("id "); }
    vga_print_str("\b) at 0x");
    vga_print_hex(faulting_address);
    vga_print_str("\n");

    halt();
}
