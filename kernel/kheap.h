#ifndef __KERNEL_KHEAP_H__
#define __KERNEL_KHEAP_H__

#include <types.h>

#define KHEAP_START         0xd0000000
#define KHEAP_INITIAL_SIZE  0x100000
#define HEAP_MIN_SIZE      0x70000
#define MAGIC               0xdeadbeef

typedef struct node
{
    uint32_t magic; /* magic number for error checking */
    uint32_t size;  /* least significant bit is used to mark block used (1) or free (0) */
    struct node *link[3]; /* left child (0), right child (1), duplicate list (2) */
} header_t;

typedef struct
{
    uint32_t magic;
    header_t *header;
} footer_t;

typedef struct
{
    struct node *root;
    size_t num_nodes;
    size_t num_duplicates;
} tree_t;

typedef struct
{
    tree_t tree;
    uintptr_t start_address;
    uintptr_t end_address;
    uintptr_t max_address;
    uint8_t supervisor;
    uint8_t readonly;
} heap_t;


heap_t *create_heap(uintptr_t start, uintptr_t end, uintptr_t max, uint8_t supervisor, uint8_t readonly);

void *alloc(uint32_t size, uint32_t alignment, heap_t *heap);
void free(void *p, heap_t *heap);

void *kmalloc(uint32_t size);
void kfree(void *p);

#endif
