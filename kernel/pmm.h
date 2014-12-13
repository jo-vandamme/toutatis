#ifndef __KERNEL_PHYS_MEM_H__
#define __KERNEL_PHYS_MEM_H__

#include <types.h>
#include <multiboot.h>

#define FRAME_SIZE 0x1000

typedef uintptr_t phys_addr_t;

int pmm_init(multiboot_info_t* mbi);

uint32_t test_frame(uint32_t frame);
void set_frame(uint32_t frame);
void clear_frame(uint32_t frame);

phys_addr_t pmm_alloc_frame();
phys_addr_t pmm_alloc_frames(size_t num);
void pmm_free_frame(phys_addr_t addr);
void pmm_free_frames(phys_addr_t addr, size_t num);

uint32_t pmm_num_free_frames();
uint32_t pmm_num_used_frames();
uint32_t pmm_memory_size();

#endif
