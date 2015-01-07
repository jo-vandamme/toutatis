#ifndef __KERNEL_MEM_ALLOC_H__
#define __KERNEL_MEM_ALLOC_H__

#include <types.h>
#include <rb_tree.h>

struct allocator;
struct page_dir;

typedef struct allocator
{
    struct rb_tree mem_tree;
    uintptr_t start_address;
    uintptr_t end_address;
    size_t min_size;
    size_t max_size;
    struct page_dir *page_dir; 
    uint8_t supervisor;
    uint8_t readonly;
} allocator_t;

allocator_t *create_mem_allocator(uintptr_t start, uintptr_t end, size_t min, size_t max, 
                                  uint8_t supervisor, uint8_t readonly, struct page_dir *dir);
void *alloc(size_t size, size_t alignment, allocator_t *allocator);
void free(void *p, allocator_t *allocator);

#endif 
