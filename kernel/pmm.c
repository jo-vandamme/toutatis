#include <pmm.h>
#include <string.h>
#include <logging.h>

static uint32_t *frames;
static uint32_t memory_size;
static uint32_t used_frames;
static uint32_t max_frames;

extern uint32_t kernel_voffset;
extern uint32_t kernel_start;
extern uint32_t kernel_end;
extern uint32_t kernel_phys;

#define FRAME_TO_IDX(frame) ((frame) / 32)
#define FRAME_TO_OFF(frame) ((frame) % 32)

uint32_t test_frame(uint32_t frame)
{
    return (frames[FRAME_TO_IDX(frame)] & (1 << FRAME_TO_OFF(frame)));
}

void set_frame(uint32_t frame)
{
    if (frame >= max_frames)
        return;

    if (!test_frame(frame))
    {
        frames[FRAME_TO_IDX(frame)] |= (1 << FRAME_TO_OFF(frame));
        ++used_frames;
    }
}

void clear_frame(uint32_t frame)
{
    if (frame >= max_frames)
        return;

    if (test_frame(frame))
    {
        frames[FRAME_TO_IDX(frame)] &= ~(1 << FRAME_TO_OFF(frame));
        --used_frames;
    }
}

static int32_t first_free_frame()
{
    uint32_t i, j;
    for (i = 0; i < max_frames; ++i)
    {
        if (frames[i] != 0xffffffff)
        {
            for (j = 0; j < 32; ++j)
            {
                if (!(frames[i] & (1 << j)))
                {
                    return i * 32 + j;
                }
            }
        }
    }
    return -1;
}

static int32_t first_free_frames(size_t num)
{
    if (num == 0)
        return -1;

    if (num == 1)
        return first_free_frame();

    uint32_t i, j, start, count;
    for (i = 0; i < max_frames; ++i)
    {
        if (frames[i] != 0xffffffff)
        {
            for (j = 0; j < 32; ++j)
            {
                if (!(frames[i] & (1 << j)))
                {
                    start = i * 32 + j;
                    for (count = 0; count < num; ++count)
                    {
                        if (test_frame(start + count))
                            break;
                    }
                    if (count == num)
                        return i * 32 + j;
                }
            }
        }
    }
    return -1;
}

static void set_region(phys_addr_t base, size_t len)
{
    /* make sure we are reserving all memory within [base - base+len] */
    uint32_t align  = base / FRAME_SIZE;
    uint32_t frames = len / FRAME_SIZE;
    if ((len + base) % FRAME_SIZE) {
        frames += 1;
    }

    for (; frames > 0; --frames, ++align) {
        set_frame(align);
    }
}

static void clear_region(phys_addr_t base, size_t len)
{
    /* make sure we are not clearing memory beyond [base - base+len] */
    uint32_t align  = base / FRAME_SIZE;
    uint32_t frames = len / FRAME_SIZE;
    if (base % FRAME_SIZE) { 
        align += 1;
        if (frames != 0)
            frames -= 1;
    }

    for (; frames > 0; --frames, ++align) {
        clear_frame(align);
    }

    /* first 4K is always set to protect the IVT, BDA, EBDA, VRAM... */
    set_frame(0);
}

int pmm_init(multiboot_info_t* mbi)
{
    /* the bitmap is placed just above the kernel, properly aligned */
    uint32_t kend = (uint32_t)&kernel_end + 1; 
    if (kend % FRAME_SIZE) /* round up to next FRAME_SIZE */
    {
        kend -= (kend % FRAME_SIZE);
        kend += FRAME_SIZE;
    }
    frames = (uint32_t *)kend;
    kprintf(INFO, "[pmm] Bitmap located at %#010x\n", frames);

    /* installed memory, calculated from the memory map structure */
    /* the bitmap represents the memory from 0 to the the end of the last available memory segment */
    mmap_entry_t *mmap = 0;
    mmap_entry_t *last_entry = 0;

    for (mmap = (mmap_entry_t *)(mbi->mmap_addr + (uint32_t)&kernel_voffset);
        (uint32_t)mmap < mbi->mmap_addr + (uint32_t)&kernel_voffset + mbi->mmap_length;
        mmap = (mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size)))
    {
        if (mmap->type == 1) {
            last_entry = mmap;
        }
    }
    if (!last_entry) {
        kprintf(CRITICAL, "\033\014No available memory\n");
        return 1;
    }
    memory_size = last_entry->addr_low + last_entry->length_low;

    /* by default, all memory is in use */
    max_frames = memory_size / FRAME_SIZE;
    used_frames = max_frames;
    memsetw(frames, 0xffff, max_frames / 16 + 1); /* 16 frames per word */

    /* free regions declared available on the memory map */
    for (mmap = (mmap_entry_t *)(mbi->mmap_addr + (uint32_t)&kernel_voffset);
        (uint32_t)mmap < mbi->mmap_addr + (uint32_t)&kernel_voffset + mbi->mmap_length;
        mmap = (mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size)))
    {
        if (mmap->type == 1) {
            clear_region(mmap->addr_low, mmap->length_low);
        }
    }

    /* reserve frames for the kernel and the bitmap */
    uint32_t len = (uint32_t)frames - (uint32_t)&kernel_start + max_frames / 8;
    set_region((uint32_t)&kernel_phys, len);

    /* reserve a frame for the multiboot info structure */
    set_region((uint32_t)mbi - (uint32_t)&kernel_voffset, sizeof(multiboot_info_t));

    kprintf(INFO, "[pmm] %u frames (%uMB)- %u used (%uKB) - %u free (%uKB)\n",
        max_frames, max_frames * FRAME_SIZE / (1024 * 1024),
        used_frames, used_frames * FRAME_SIZE / 1024,
        pmm_num_free_frames(), pmm_num_free_frames() * FRAME_SIZE / 1024);

    return 0;
}

phys_addr_t pmm_alloc_frame()
{
    int frame;
    if ((pmm_num_free_frames() == 0) ||
        ((frame = first_free_frame()) == -1))
    {
        kprintf(CRITICAL, "\033\014No more memory\n");
        return 0;
    }

    set_frame(frame);

    return (phys_addr_t)(frame * FRAME_SIZE);
}

phys_addr_t pmm_alloc_frames(size_t num)
{
    int frame;
    if ((pmm_num_free_frames() == 0) ||
        ((frame = first_free_frames(num)) == -1))
    {
        kprintf(CRITICAL, "\033\014No more memory\n");
        return 0;
    }

    for (uint32_t i = 0; i < num; ++i)
        set_frame(frame + i);

    return (phys_addr_t)(frame * FRAME_SIZE);
}

void pmm_free_frame(phys_addr_t addr)
{
    clear_frame(addr / FRAME_SIZE);
}

void pmm_free_frames(phys_addr_t addr, size_t num)
{
    for (uint32_t i = 0; i < num; ++i)
        clear_frame(addr / FRAME_SIZE + i);
}

uint32_t pmm_num_free_frames()
{
    return max_frames - used_frames;
}

uint32_t pmm_num_used_frames()
{
    return used_frames;
}

uint32_t pmm_memory_size()
{
    return memory_size;
}
