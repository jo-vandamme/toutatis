#ifndef __KERNEL_KHEAP_H__
#define __KERNEL_KHEAP_H__

#include <types.h>
#include <mem_alloc.h> // XXX: to be removed

#define KHEAP_START         0xd0000000
#define KHEAP_INITIAL_SIZE  0x00100000
#define HEAP_MIN_SIZE       0x00070000
#define HEAP_MAX_SIZE       0x00f00000

void kheap_init(); // XXX: this should be called instead of create_mem_allocator
void *kmalloc(uint32_t size);
void *kmalloc_a(uint32_t size);
void *kmalloc_p(uint32_t size, uintptr_t *phys);
void *kmalloc_ap(uint32_t size, uintptr_t *phys);
void kfree(void *p);

#endif
