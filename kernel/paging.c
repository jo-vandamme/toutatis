#include <system.h>
#include <paging.h>
#include <logging.h>
#include <string.h>
#include <kheap.h>

#define BIT_TO_IDX(bit) ((bit) / 32)
#define BIT_TO_OFF(bit) ((bit) % 32)

#define PAGE_DIRECTORY_INDEX(virt)  (((virt) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(virt)      (((virt) >> 12) & 0x3ff)
#define PAGE_FRAME(virt)            ((virt) & 0xfff);

extern uintptr_t kernel_end;
extern uintptr_t kernel_voffset;

static uint32_t *frames;
static uint32_t nframes;
static uint32_t used_frames;

page_dir_t *current_directory = 0;
page_dir_t *kernel_directory = 0;

extern uintptr_t placement_address;
extern allocator_t *kheap;

inline int test_frame(uint32_t frame)
{
    return (frames[BIT_TO_IDX(frame)] & (1 << BIT_TO_OFF(frame)));
}

inline void set_frame(uint32_t frame)
{
    if (test_frame(frame) == 0) {
        frames[BIT_TO_IDX(frame)] |= (1 << BIT_TO_OFF(frame));
        ++used_frames;
    }
}

inline void clear_frame(uint32_t frame)
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

/* get a page based on a virtual address and a specific page directory,
 * the page directory entry and page table will be created if necessary
 * when the flag make is true */
pte_t *get_page(uintptr_t virt, int make, page_dir_t *dir)
{
    assert(dir != NULL);

    uint32_t dir_idx = PAGE_DIRECTORY_INDEX(virt);
    if (dir->tables[dir_idx]) {
        return &dir->tables[dir_idx]->pages[PAGE_TABLE_INDEX(virt)];
    }
    else if (make) {
        /* page table not present, allocate and clear it */
        uintptr_t phys;
        dir->tables[dir_idx] = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &phys);
        memset(dir->tables[dir_idx], 0, sizeof(page_table_t));

        /* map the page table */
        dir->entries[dir_idx].present = 1;
        dir->entries[dir_idx].read_write = 1;
        dir->entries[dir_idx].user_supervisor = 1;
        dir->entries[dir_idx].page_table_base = phys >> 12;
        return &dir->tables[dir_idx]->pages[PAGE_TABLE_INDEX(virt)];
    }
    else {
        kprintf(ERROR, "\033\014Page directory entry not present\n");
        return 0;
    }
}

void paging_init(uint32_t mem_size)
{
    /* the bitmap is placed just above the kernel, properly aligned */
    nframes = mem_size / FRAME_SIZE;
    frames = (uint32_t *)kmalloc_a(nframes / 8); /* 1 byte = 8 frames */
    memset(frames, 0, nframes / 8);
    used_frames = 0;

    kprintf(INFO, "[paging] Frames bitmap located at %#010x\n", frames);
}

void paging_finalize()
{
    /* first 4K is always set to protect the IVT, BDA, EBDA, VRAM... */
    set_frame(0);

    /* create kernel directory table */
    uintptr_t phys;
    kernel_directory = (page_dir_t *)kmalloc_ap(sizeof(page_dir_t), &phys);
    memset(kernel_directory, 0, sizeof(page_dir_t));
    kernel_directory->entries_phys_addr = phys;

    /* map some pages in the kernel heap area.
     * here we call get_page but not alloc_page. This causes
     * page tables to be created where necessary. We can't allocate frames
     * yet because they need to be identity mapped first below, and yet we can't
     * increase placement_address between identity mapping and enabling the heap! */
    uint32_t virt = 0;
    for (virt = KHEAP_START;
         virt < KHEAP_START + KHEAP_INITIAL_SIZE;
         virt += FRAME_SIZE) 
    {
        get_page(virt, 1, kernel_directory);
    }

    /* map kernel and bitmap (above 3GB) to low memory */
    phys = 0;
    virt = (uintptr_t)&kernel_voffset;
    /* add a few frames to the placement address for the clone operation */
    while (virt < placement_address + FRAME_SIZE*4)
    {
        //kprintf(INFO, "=> mapping %x to %x\n", virt, phys);
        map_page(get_page(virt, 1, kernel_directory), 0, 0, phys);
        phys += FRAME_SIZE;
        virt += FRAME_SIZE;
    }

    /* allocate pages for the kernel heap */
    for (virt = KHEAP_START; 
         virt < KHEAP_START + KHEAP_INITIAL_SIZE;
         virt += FRAME_SIZE)
    {
        alloc_page(get_page(virt, 1, kernel_directory), 0, 1);
    }

    /* before we enable paging, we must register the page fault handler */
    attach_interrupt_handler(14, page_fault);

    /* switch to our page directory */
    current_directory = clone_page_directory(kernel_directory);
    switch_page_directory(current_directory);
    kprintf(INFO, "[paging] Paging installed\n");

    /* initialize the kernel heap */
    kheap = create_mem_allocator(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, 
            HEAP_MIN_SIZE, HEAP_MAX_SIZE, 0, 0, kernel_directory);
    //kheap = create_heap(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, 
    //        KHEAP_START + HEAP_MAX_SIZE, 0, 0);

    kprintf(INFO, "[paging] %u frames (%uMB) - %u used (%uKB) - %u free (%uMB)\n",
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
                  :: "r"(dir->entries_phys_addr)
                  : "%eax");
    return 1;
}

void invalidate_page_tables_at(uintptr_t addr)
{
    asm volatile ("movl %0, %%eax\n"
                  "invlpg (%%eax)\n"
                  :: "r"(addr) : "%eax");
}

static void copy_frame(uintptr_t src, uintptr_t dst)
{
    kprintf(INFO, "\033\012Copying %x to %x\033\017\n", src, dst);
    /* find two regions of virtual memory that are not mapped
     * and map them to both physical frames in order to do the copy
     */
    uintptr_t region_src = 0xe0000000;
    uintptr_t region_dst = 0xe0000000;
    (void)region_dst;

    uint32_t idx = PAGE_DIRECTORY_INDEX(region_src);
    kprintf(INFO, "idx = %x\n", idx);
    kprintf(INFO, "\033\014Implement this\n");
    stop();
}

static page_table_t *clone_page_table(page_table_t *table, uintptr_t *phys)
{
    kprintf(INFO, "cloning table %x\n", table);
    page_table_t *clone = (page_table_t *)kmalloc_ap(sizeof(page_table_t), phys);
    memset(clone, 0, sizeof(page_table_t));

    for (int i = 0; i < 1024; ++i) {
        if (table->pages[i].frame) {
            /* allocate a new frame */
            alloc_page(&clone->pages[i], 0, 0);
            /* clone the flags */
            uint32_t frame = clone->pages[i].frame;
            clone->pages[i] = table->pages[i];
            clone->pages[i].frame = frame;
            /* physically copy the data */
            copy_frame(table->pages[i].frame * FRAME_SIZE, 
                       clone->pages[i].frame * FRAME_SIZE);
        }
    }
    return clone;
}

page_dir_t *clone_page_directory(page_dir_t *dir)
{
    uintptr_t phys;
    page_dir_t *clone = (page_dir_t *)kmalloc_ap(sizeof(page_dir_t), &phys);
    memset(clone, 0, sizeof(page_dir_t));
    clone->entries_phys_addr = phys;

    /* clone the page tables */
    for (int i = 0; i < 1024; ++i) {
        if (!dir->tables[i]) { 
            continue; 
        }
        if (kernel_directory->tables[i] == dir->tables[i]) {
            kprintf(INFO, "linking dir->tables[%x] = %x\n", i, dir->tables[i]);
            clone->tables[i] = dir->tables[i];
            clone->entries[i] = dir->entries[i];
        }
        else {
            uintptr_t phys;
            kprintf(INFO, "cloning table dir->tables[%x]\n", i);
            clone->tables[i] = clone_page_table(dir->tables[i], &phys);
            //clone->entries[i].present = 1;
            //clone->entries[i].read_write = 1;
            //clone->entries[i].user_supervisor = 1;
            clone->entries[i] = dir->entries[i];
            clone->entries[i].page_table_base = phys >> 12;
        }
    }
    return clone;
}

void page_fault(registers_t *regs)
{
    cli();
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    int present  = !(regs->err_code & (1 << 0)); // page not present
    int rw       = regs->err_code & (1 << 1);    // write operation
    int us       = regs->err_code & (1 << 2);    // processor was in user-mode
    int reserved = regs->err_code & (1 << 3);    // overwritten cpu-reserved bits of pte
    int id       = regs->err_code & (1 << 4);    // occured during an instruction fetch

    kprintf(ERROR, "\033\014Page fault! (");
    if (present)  { kprintf(ERROR, "not present "); }
    if (rw)       { kprintf(ERROR, "read-only "); }
    if (us)       { kprintf(ERROR, "user-mode "); }
    if (reserved) { kprintf(ERROR, "reserved "); }
    if (id)       { kprintf(ERROR, "id "); }
    kprintf(ERROR, "\b) at 0x%x\n", faulting_address);

    kprintf(ERROR,
            "eax: %#010x ebx: %#010x\n"
            "ecx: %#010x edx: %#010x\n"
            "esi: %#010x edi: %#010x\n"
            "ebp: %#010x esp: %#010x\n"
            "eip: %#010x efl: %#010x\n"
            "ss: %#04x cs: %#04x ds: %#04x\n"
            "es: %#04x fs: %#04x gs: %#04x\n\033\017",
            regs->eax, regs->ebx, regs->ecx, regs->edx,
            regs->esi, regs->edi, regs->ebp, regs->esp,
            regs->eip, regs->eflags,
            regs->ss, regs->cs, regs->ds,
            regs->es, regs->fs, regs->gs);

    stop();
}
