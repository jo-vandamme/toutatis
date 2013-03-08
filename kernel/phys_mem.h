#ifndef __KERNEL_PHYS_MEM_H__
#define __KERNEL_PHYS_MEM_H__

#include <types.h>
#include <multiboot.h>

#define BLOCK_SIZE 0x1000

void pmm_init(multiboot_info_t* mbi, size_t mem_size, size_t kernel_size);

addr_t pmm_alloc_block();
void pmm_free_block(addr_t addr);

u32_t pmm_num_free_blocks();
u32_t pmm_num_used_blocks();
u32_t pmm_memory_size();

#endif
