#include <phys_mem.h>
#include <string.h>
#include <logging.h>

static u32_t memory_size;
static u32_t used_blocks;
static u32_t max_blocks;

static u32_t *frames;
static u32_t last_alloc_block = 0;

static void set_block(u32_t frame)
{
        frames[frame / 32] |= (1 << (frame % 32));
}

static void clear_block(u32_t frame)
{
        frames[frame / 32] &= ~(1 << (frame % 32));
}

/*
static u8_t test_block(u32_t frame)
{
        return (frames[frame / 32] & (1 << (frame % 32)));
}*/

static int first_block()
{
        u32_t i, j;
        u32_t last_frame = last_alloc_block / 32;
        for (i = last_frame; (i - last_frame) < max_blocks; ++i, i %= max_blocks) {
                if (frames[i] != 0xffffffff) {
                        for (j = 0; j < 32; ++j) {
                                u32_t bit = 1 << j;
                                if (!(frames[i] & bit)) {
                                        last_alloc_block = i * 32 + j;
                                        return last_alloc_block;
                                }
                        }
                }
        }
        return -1;
}

static void set_region(addr_t base, size_t len)
{
        u32_t blocks = len / BLOCK_SIZE;
        u32_t align  = base / BLOCK_SIZE;

        for (; blocks > 0; --blocks, ++align) {
                set_block(align);
                ++used_blocks;
        }
}

static void clear_region(addr_t base, size_t len)
{
        u32_t blocks = len / BLOCK_SIZE;
        u32_t align  = base / BLOCK_SIZE;

        for (; blocks > 0; --blocks, ++align) {
                clear_block(align);
                --used_blocks;
        }

        /* first 4K is always set to protect the IVT, BDA, EBDA, VRAM... */
        set_block(0);
}

void pmm_init(multiboot_info_t* mbi, size_t mem_size, size_t kernel_size)
{
        /* round up kernel size to next BLOCK_SIZE */
        u32_t ksize = kernel_size;
        if (ksize % BLOCK_SIZE) {
                ksize -= (ksize % BLOCK_SIZE);
                ksize += BLOCK_SIZE;
        }
        frames = (u32_t *)(0xc0000000 + ksize);
        memory_size = mem_size;
        max_blocks = memory_size / BLOCK_SIZE;
        used_blocks = max_blocks;

        /* By default, all memory is in use */
        memsetw(frames, 0xffff, max_blocks / 16); /* 16 blocks per word */

        /* Free available regions */
        mmap_entry_t * mmap = (mmap_entry_t *)mbi->mmap_addr;
        while ((u32_t)mmap < mbi->mmap_addr + mbi->mmap_length)
        {
                if (mmap->type > 4)
                        mmap->type = 1;

                if (mmap->type == 1) {
                        clear_region((mmap->addr_high << 8) + mmap->addr_low,
                                     (mmap->length_high << 8) + mmap->length_low);
                }
                mmap = (mmap_entry_t *)((u32_t)mmap + 24);
        }
        /* reserve blocks for kernel and frames */
        u32_t msize = mem_size / BLOCK_SIZE / 8;
        if (msize % BLOCK_SIZE) {
                msize -= (msize % BLOCK_SIZE);
                msize += BLOCK_SIZE;
        }
        set_region(0x100000, ksize + msize);

        kprintf(INFO, "\033\010[pmm] \033\007%u\033\010 blocks of %uKB - \033\007%u\033\010 used - \033\007%u\033\010 free\n",
                max_blocks, BLOCK_SIZE / 1024, used_blocks, pmm_num_free_blocks());
}

addr_t pmm_alloc_block()
{
        int frame;
        if ((pmm_num_free_blocks() == 0) ||
            ((frame = first_block()) == -1)) {
                return 0;
        }

        set_block(frame);

        ++used_blocks;
        return (addr_t)(frame * BLOCK_SIZE);
}

void pmm_free_block(addr_t addr)
{
        clear_block(addr / BLOCK_SIZE);
        --used_blocks;
}

u32_t pmm_num_free_blocks()
{
        return max_blocks - used_blocks;
}

u32_t pmm_num_used_blocks()
{
        return used_blocks;
}

u32_t pmm_memory_size()
{
        return memory_size;
}
