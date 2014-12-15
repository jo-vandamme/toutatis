#include <system.h>
#include <paging.h>
#include <logging.h>
#include <vga.h>
#include <string.h>

#define BIT_TO_IDX(bit) ((bit) / 32)
#define BIT_TO_OFF(bit) ((bit) % 32)

#define PAGE_DIRECTORY_INDEX(virt) (((virt) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(virt)     (((virt) >> 12) & 0x3ff)
#define PAGE_FRAME(virt)           ((virt) & 0xfff);

extern uint32_t kernel_end;
extern uint32_t kernel_voffset;

static uint32_t *frames;
static uint32_t nframes;
static uint32_t used_frames;

page_dir_t *current_directory = 0;
page_dir_t *kernel_directory = 0;

int test_frame(uint32_t frame)
{
    return (frames[BIT_TO_IDX(frame)] & (1 << BIT_TO_OFF(frame)));
}

void set_frame(uint32_t frame)
{
    if (test_frame(frame) == 0) {
        frames[BIT_TO_IDX(frame)] |= (1 << BIT_TO_OFF(frame));
        ++used_frames;
    }
}

void clear_frame(uint32_t frame)
{
    if (test_frame(frame) == 1) {
        frames[BIT_TO_IDX(frame)] &= ~(1 << BIT_TO_OFF(frame));
        --used_frames;
    }
}

int32_t first_free_frames(size_t num)
{
    if (num == 0)
        return -1;

    uint32_t i, j, start, count;

    for (i = 0; i < nframes; ++i) {
        if (frames[i] != 0xffffffff) {
            for (j = 0; j < 32; ++j) {
                if (!(frames[i] & (1 << j))) {
                    start = i * 32 + j;
                    for (count = 1; count < num; ++count) {
                        if (test_frame(start + count)) {
                            break;
                        }
                    }
                    if (count == num) {
                        return i * 32 + j;
                    }
                }
            }
        }
    }
    kprintf(CRITICAL, "Out of usable memory\n");
    return -1;
}

int32_t first_free_frame()
{
    return first_free_frames(1);
}

uint32_t memory_used()
{
    uint32_t i, j, count = 0;

    for (i = 0; i < nframes; ++i) {
        for (j = 0; j < 32; ++j) {
            if (frames[i] & (1 << j)) {
                ++count;
            }
        }
    }
    return count * FRAME_SIZE;
}

uint32_t memory_total()
{
    return nframes * FRAME_SIZE;
}

void paging_mark_reserved(uintptr_t address)
{
    set_frame(address / FRAME_SIZE);
}

void alloc_page(pte_t *page, int is_kernel, int is_writeable)
{
    /* if the frame was alread allocated, keep it */
    if (page->frame != 0) {
        page->present = 1;
        page->read_write = (is_writeable) ? 1 : 0;
        page->user_supervisor = (is_kernel) ? 0 : 1;
        return;
    }

    /* allocate a free physical frame */
    int32_t frame = first_free_frame();
    if (frame == -1) {
        kprintf(CRITICAL, "Out of frames\n");
        return;
    }
    set_frame(frame);

    /* map the frame to the page */
    page->frame = frame;
    page->present = 1;
    page->read_write = (is_writeable) ? 1 : 0;
    page->user_supervisor = (is_kernel) ? 0 : 1;
}

void map_page(pte_t *page, int is_kernel, int is_writeable, uintptr_t phys)
{
    uint32_t frame = phys / FRAME_SIZE;

    page->present = 1;
    page->read_write = (is_writeable) ? 1 : 0;
    page->user_supervisor = (is_kernel) ? 0 : 1;
    page->frame = frame;

    if (frame < nframes) {
        set_frame(frame);
    }
}

void free_page(pte_t *page)
{
    if (page->frame) {
        clear_frame(page->frame);
    }
    page->present = 0;
    page->frame = 0x0;
}

pte_t *get_page(uintptr_t virt, int make, page_dir_t *dir)
{
    /* get page table entry */
    pde_t *entry = &dir->tables[PAGE_DIRECTORY_INDEX(virt)];

    /* get the page table and create it first if necessary */
    page_table_t *table;
    if (entry->present) {
        table = (page_table_t *)(entry->page_table_base << 12);
    }
    else if (make) {
        /* page table not present, allocate it */
        int32_t frame = first_free_frame(); 
        if (frame == -1) {
            return 0;
        }
        set_frame(frame);
        table = (page_table_t *)((uintptr_t)(frame * FRAME_SIZE));

        /* clear page table */
        memset(table, 0, sizeof(page_table_t));

        /* map the page table */
        entry->present = 1;
        entry->read_write = 1;
        entry->page_table_base = ((uintptr_t)table) >> 12;
    }
    else {
        return 0;
    }

    return &table->pages[PAGE_TABLE_INDEX(virt)];
}

void paging_init(uint32_t mem_size)
{
    /* the bitmap is placed just above the kernel, properly aligned */
    uint32_t kend = (uint32_t)&kernel_end + 1; 
    if (kend % FRAME_SIZE) /* round up to next FRAME_SIZE */
    {
        kend -= (kend % FRAME_SIZE);
        kend += FRAME_SIZE;
    }
    frames = (uint32_t *)kend;
    nframes = mem_size / FRAME_SIZE;
    used_frames = 0;
    memset(frames, 0, nframes);

    kprintf(INFO, "[paging] Bitmap located at %#010x\n", frames);
}

void paging_finalize()
{
    /* first 4K is always set to protect the IVT, BDA, EBDA, VRAM... */
    set_frame(0);

    /* create kernel directory table */
    int32_t frame = first_free_frame();
    if (frame == -1) {
        return;
    }
    set_frame(frame);
    kernel_directory = (page_dir_t *)((uintptr_t)(frame * FRAME_SIZE));
    memset(kernel_directory, 0, sizeof(page_dir_t));

    /* map kernel and bitmap (above 3GB) to low memory */
    for (uint32_t phys = 0, virt = (uint32_t)&kernel_voffset;
         virt <= (uintptr_t)frames + nframes / 8; 
         phys += FRAME_SIZE, virt += FRAME_SIZE) 
    {
        map_page(get_page(virt, 1, kernel_directory), 1, 0, phys);
    }
    kprintf(INFO, "mapping up to %#010x\n", (uintptr_t)frames + nframes / 8);

    /* before we enable paging, we must register the page fault handler */
    attach_interrupt_handler(14, page_fault);

    /* switch to our page directory */
    switch_page_directory(kernel_directory);

    kprintf(INFO, "[paging] %u frames (%uMB)- %u used (%uKB) - %u free (%uMB)\n",
        nframes, nframes * FRAME_SIZE / (1024 * 1024),
        used_frames, used_frames * FRAME_SIZE / 1024,
        nframes - used_frames, (nframes - used_frames) * FRAME_SIZE / (1024 * 1024));
}

int switch_page_directory(page_dir_t *dir)
{
    if (!dir) {
        return 0;
    }
    current_directory = dir;

    asm volatile ("mov %0, %%cr3\n"     /* set page directory base register (PDBR) */
                  "mov %%cr0, %%eax\n"
                  "orl $0x80000000, %%eax\n"
                  "mov %%eax, %%cr0\n"   /* enable paging */
                  :: "r"((uintptr_t)&dir->tables)
                  : "%eax");
    return 1;
}

void invalidate_page_tables_at(uintptr_t addr)
{
    asm volatile ("movl %0, %%eax\n"
                  "invlpg (%%eax)\n"
                  :: "r"(addr) : "%eax");
}

void page_fault(registers_t *regs)
{
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

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

    HALT;
}
