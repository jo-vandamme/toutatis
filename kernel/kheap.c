#include <system.h>
#include <paging.h>
#include <logging.h>
#include <kheap.h>
#include <mem_alloc.h>

extern uint32_t kernel_end;
extern uint32_t kernel_voffset;
extern page_dir_t *kernel_directory; 

uintptr_t placement_address = (uintptr_t)&kernel_end;
allocator_t *kheap = 0;

static uint8_t volatile mem_lock = 0;

static uintptr_t kmalloc_int(uint32_t size, uint32_t alignment, uintptr_t *phys)
{
    spin_lock(&mem_lock);

    if (kheap != 0) {
        kprintf(INFO, "\n------------------ alloc(%x) -------------------\n", size);
        void *addr = alloc(size, alignment, kheap);
        if (phys != 0) {
            pte_t *page = get_page((uintptr_t)addr, 0, kernel_directory);
            *phys = page->frame * FRAME_SIZE + ((uintptr_t)addr & 0xfff);
        }
        spin_unlock(&mem_lock);
        return (uintptr_t)addr;
    }
    else {
        if (alignment != 0 && (placement_address % alignment)) {
            placement_address -= (placement_address % alignment);
            placement_address += alignment;
        }
        if (phys) {
            *phys = placement_address - (uintptr_t)&kernel_voffset;
        }
        uintptr_t tmp = placement_address;
        placement_address += size;

        spin_unlock(&mem_lock);
        return tmp;
    }
}

inline void *kmalloc(uint32_t size)
{
    return (void *)kmalloc_int(size, 0, 0);
}

inline void *kmalloc_a(uint32_t size)
{
    return (void *)kmalloc_int(size, FRAME_SIZE, 0);
}

inline void *kmalloc_p(uint32_t size, uintptr_t *phys)
{
    return (void *)kmalloc_int(size, 0, phys);
}

inline void *kmalloc_ap(uint32_t size, uintptr_t *phys)
{
    return (void *)kmalloc_int(size, FRAME_SIZE, phys);
}

inline void kfree(void *p)
{
    spin_lock(&mem_lock);
    kprintf(INFO, "\n--------------- free(%x) ---------------\n", p);
    free(p, kheap);
    spin_unlock(&mem_lock);
}
