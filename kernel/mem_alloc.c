#include <rb_tree.h>
#include <paging.h>
#include <logging.h>
#include <mem_alloc.h>

#define MAGIC                       (uint32_t)0xa1b2c3d4 /* last bit is ignored */
#define set_magic(block)            ((block)->magic = ((MAGIC & ~(1 << 0)) | ((block)->magic & (1 << 0))))
#define check_magic(block)          (((block)->magic & ~(1 << 0)) == (MAGIC & ~(1 << 0)))
#define is_free(block)              ((block)->magic &   (1 << 0)) /* last bit used for free/used status of block */
#define mark_free(block)            ((block)->magic |=  (1 << 0))
#define mark_used(block)            ((block)->magic &= ~(1 << 0))

#define rounded_size(size)          ((((size) + sizeof(long) - 1) / sizeof(long)) * sizeof(long))
#define MIN_BLOCK_SIZE              rounded_size(sizeof(alloc_header_t) + sizeof(alloc_footer_t))

#define get_size(block)             ((size_t)(block)->rb_node.data)
#define set_size(block, size)       ((block)->rb_node.data = (void *)(size))

#define USER_PTR_OFFSET             (sizeof(((alloc_header_t *)0)->magic) + sizeof(((rb_node_t *)0)->data))
#define rbnode_to_user(node)        ((uintptr_t)(node) + sizeof(((rb_node_t *)0)->data))
#define rbnode_to_block(node)       ((alloc_header_t *)((uintptr_t)node - sizeof(((alloc_header_t *)0)->magic)))
#define block_to_rbnode(block)      ((rb_node_t *)((uintptr_t)(block) + sizeof(((alloc_header_t *)0)->magic)))
#define block_to_user(block)        ((uintptr_t)(block) + (uintptr_t)USER_PTR_OFFSET)
#define user_to_block(ptr)          ((alloc_header_t *)((uintptr_t)(ptr) - USER_PTR_OFFSET))

#define get_footer(block)           ((alloc_footer_t *)((uintptr_t)(block) + get_size(block) - sizeof(alloc_footer_t)))
#define get_left_footer(block)      ((alloc_footer_t *)((uintptr_t)(block) - sizeof(alloc_footer_t)))
#define get_next_block(block)       ((alloc_header_t *)((uintptr_t)(block) + get_size(block)))

typedef struct
{
    uint32_t magic;
    rb_node_t rb_node;
} alloc_header_t;

typedef struct 
{
    uint32_t magic;
    alloc_header_t *header;
} alloc_footer_t;

struct alloc_args
{
    size_t alignment; /* > 0 for best_fit */
    uintptr_t address;
};

/* size is the size requested by the user
 * returns the size of the best fitting block
 */
static inline size_t get_block_size(const size_t size)
{
    size_t s = rounded_size(size + USER_PTR_OFFSET + sizeof(alloc_footer_t));
    return s < MIN_BLOCK_SIZE ? MIN_BLOCK_SIZE : s;
}

/* retruns -1 if node < data => gives the search direction
 *          0 if the node is a possible match
 *          1 if node > data
 */
int compare(const rb_node_t *node, const void *data, const void *args)
{
    /* node is too small */
    if (node->data < data) {
        return -1;
    }

    size_t alignment;
    if (args && (alignment = ((struct alloc_args *)args)->alignment) != 0) {
        /* best fit for required alignment
         * the aligment is with respect to the user pointer */
        size_t offset = alignment - (size_t)rbnode_to_user(node) % alignment;
        offset %= alignment;

        while (offset && offset < MIN_BLOCK_SIZE) {
            offset += alignment;
        }
        /* node is too small */
        if ((uintptr_t)node->data - offset < (uintptr_t)data) {
            return -1;
        /* node is big enough - potential candidate */
        } else {
            return 0; 
        }
    } else { /* node is too big */
        if (node->data > data) {
            return 1;
        } else { /* exact match */
            return 0;
        }
    }
}

int select_dup(const rb_node_t *node, const void *args)
{
    if (!node) {
        return 0;
    }

    uintptr_t address;
    if (args && (address = ((struct alloc_args *)args)->address) != 0
        && (uintptr_t)node != address) {
        return 0;
    }
    return 1;
}

static void expand(uintptr_t new_end_address, allocator_t *allocator)
{
    if (new_end_address <= allocator->end_address) {
        kprintf(ERROR, "\033\017Heap expansion must be bigger than actual size!\n\033\017");
        return;
    }

    if (new_end_address % FRAME_SIZE) {
        new_end_address -= (new_end_address % FRAME_SIZE);
        new_end_address += FRAME_SIZE;
    }

    if (new_end_address > allocator->start_address + allocator->max_size) {
        kprintf(ERROR, "\033\014Heap expansion overflow!\n\033\017");
        return;
    }

    /* allocate some pages */
    while (allocator->end_address < new_end_address) {
        alloc_page(get_page(allocator->end_address, 1, allocator->page_dir),
                (allocator->supervisor) ? 1 : 0, (allocator->readonly) ? 0 : 1);
        allocator->end_address += FRAME_SIZE;
    }
}

static uintptr_t contract(uintptr_t new_end_address, allocator_t *allocator)
{
    if (new_end_address >= allocator->end_address) {
        kprintf(ERROR, "\033\014Heap contraction must be smaller than actual size\n\033\017");
        return 0;
    }

    if (new_end_address % FRAME_SIZE) {
        new_end_address -= (new_end_address % FRAME_SIZE);
        new_end_address += FRAME_SIZE;
    }

    if (new_end_address - allocator->start_address < allocator->min_size) {
        new_end_address = allocator->start_address + allocator->min_size;
    }

    while (allocator->end_address > new_end_address) {
        free_page(get_page(allocator->end_address - FRAME_SIZE, 0, allocator->page_dir));
        allocator->end_address -= FRAME_SIZE;
    }

    return allocator->end_address;
}

void *alloc(const size_t size, size_t alignment, allocator_t *allocator)
{
    /* space to store user data and block metadata */
    size_t requested_size = get_block_size(size);

    /* get a block of size "size" or bigger */
    alignment = alignment == 0 ? 1 : alignment; /* must be > 0 for best fit search */
    struct alloc_args args = { alignment, 0 }; /* no address matching */
    rb_node_t *node = remove_rbnode(&allocator->mem_tree, (void *)requested_size, &args); 

    if (!node) {
        DBPRINT("- \033\012Expansion\033\017\n");
        /* expand the heap */
        uintptr_t old_end_address = allocator->end_address;
        expand(allocator->end_address + requested_size, allocator);

        /* create a block with the added space */
        alloc_header_t *hole = (alloc_header_t *)old_end_address;
        set_size(hole, allocator->end_address - old_end_address);
        set_magic(hole);
        alloc_footer_t *hole_footer = get_footer(hole);
        hole_footer->header = hole;
        set_magic(hole_footer);

        /* try to merge left */
        alloc_footer_t *left_footer = get_left_footer(old_end_address);
        if ((uintptr_t)left_footer->header >= allocator->start_address /* node is part of the heap */
            && check_magic(left_footer)                           /* node isn't corrupted */
            && check_magic(left_footer->header)                   /* node isn't corrupted */
            && is_free(left_footer->header))                      /* node is free */
        {
            /* no aligment (disables best fit) and match address */
            struct alloc_args args = { 0, (uintptr_t)block_to_rbnode(left_footer->header) };
            rb_node_t *left_node = remove_rbnode(&allocator->mem_tree, (void *)get_size(left_footer->header), &args);

            if (!left_node) {
                kprintf(ERROR, "\033\014ERROR: expansion failed, left footer not found\n\033\017");
                return 0;
            }
            //kprintf(INFO, "merging left\n");

            /* expand block */
            alloc_header_t *left_block = rbnode_to_block(left_node);
            set_size(left_block, get_size(left_block) + get_size(hole));
            hole_footer->header = left_block;
            
            /* insert node back into the tree */
            mark_free(left_block);
            insert_rbnode(&allocator->mem_tree, left_node, (void *)0);
        } else {
            mark_free(hole);
            insert_rbnode(&allocator->mem_tree, block_to_rbnode(hole), (void *)0);
        }

        /* recurse, with a bigger heap this time */
        return alloc(size, alignment, allocator);
    }

    alloc_header_t *block = rbnode_to_block(node);
    
    /* align block if required */
    if ((size_t)block_to_user(block) % alignment) {

        size_t offset = alignment - ((size_t)block_to_user(block)) % alignment;
        while (offset < MIN_BLOCK_SIZE) {
            offset += alignment;
        }
        alloc_header_t *new_block = (alloc_header_t *)((uintptr_t)block + offset);
        set_size(new_block, get_size(block) - offset);
        alloc_footer_t *footer = get_footer(new_block);
        footer->header = new_block;
        set_magic(footer);

        assert(offset >= MIN_BLOCK_SIZE && "offset < MIN_BLOCK_SIZE"); /* XXX */

        /* return preceeding block to tree */
        //if (offset >= MIN_BLOCK_SIZE) {
        set_magic(block);
        set_size(block, offset);
        footer = get_footer(block);
        footer->header = block;
        set_magic(footer);

        mark_free(block);
        insert_rbnode(&allocator->mem_tree, block_to_rbnode(block), (void *)0);
        //}
        block = new_block;
    }

    /* fragment node if necessary */
    size_t block_size = get_size(block);
    size_t frag_size = block_size - requested_size;
    if (frag_size > 0 && frag_size >= MIN_BLOCK_SIZE) {

        /* set header and footer of user block and mark it as used */
        set_size(block, requested_size);
        set_magic(block);
        alloc_footer_t *footer = get_footer(block);
        footer->header = block;
        set_magic(footer);

        /* set header and footer of leftover and insert it back in the tree */
        alloc_header_t *next_block = get_next_block(block);
        set_size(next_block, frag_size);
        set_magic(next_block);
        footer = get_footer(next_block);
        footer->header = next_block;
        set_magic(footer);

        mark_free(next_block);
        insert_rbnode(&allocator->mem_tree, block_to_rbnode(next_block), (void *)0);
    }
    mark_used(block);
    allocator->mem_used += get_size(block);
    return (void *)block_to_user(block);
}

void free(void *p, allocator_t *allocator)
{
    if (p == NULL) {
        return;
    }

    /* get node from user pointer */
    alloc_header_t *block = user_to_block(p);

    DBPRINT("magic: %x\n", block->magic);
    assert(check_magic(block) && "Wrong header magic");

    /* don't free a node already freed */
    if (is_free(block)) {
        kprintf(ERROR, "\033\014Error: node already free\n\033\017");
        return;
    } else {
        mark_free(block);
    }

    assert(check_magic(get_footer(block)) && "Wrong footer magic");
    allocator->mem_used -= get_size(block);

    /* right neighbour merge */
    alloc_header_t *neighbour = get_next_block(block);
    if ((uintptr_t)neighbour + get_size(neighbour) <= allocator->end_address /* node is part of the heap */
        && check_magic(neighbour)               /* node isn't corrupted */
        && check_magic(get_footer(neighbour))   /* node isn't corrupted */
        && is_free(neighbour))                  /* node is free */
    {
        struct alloc_args args = { 0, (uintptr_t)block_to_rbnode(neighbour) };
        if (!remove_rbnode(&allocator->mem_tree, (void *)get_size(neighbour), &args)) {
            kprintf(ERROR, "\033\014Error: Right neighbour not found in RB-tree!\n\033\017");
            return;
        }
        //kprintf(INFO, "right merge\n");
        set_size(block, get_size(block) + get_size(neighbour));
        mark_free(block);
        get_footer(block)->header = block;
    }

    /* left neighbour merge */
    alloc_footer_t *footer = get_left_footer(block);
    if ((uintptr_t)footer->header >= allocator->start_address /* node is part of the heap */
        && check_magic(footer)                  /* node isn't corrupted */
        && check_magic(footer->header)          /* node isn't corrupted */
        && is_free(footer->header))             /* node is free */
    {
        neighbour = footer->header;
        struct alloc_args args = { 0, (uintptr_t)block_to_rbnode(neighbour) };
        if (!remove_rbnode(&allocator->mem_tree, (void *)get_size(neighbour), &args)) {
            kprintf(ERROR, "\033\014Error: Left neighbour not found in RB-tree!\n\033\017");
            return;
        }
        //kprintf(INFO, "left merge\n");
        set_size(neighbour, get_size(neighbour) + get_size(block));
        mark_free(neighbour);
        get_footer(neighbour)->header = neighbour;
        block = neighbour;
    }

    uint8_t insert = 1;
    /* if the footer location is the end address, we can contract */
    if ((uintptr_t)get_footer(block) + sizeof(alloc_footer_t) == allocator->end_address
        && allocator->end_address - allocator->start_address > allocator->min_size) {
        size_t block_size = get_size(block); /* node may be freed entirely, so save its size */
        uintptr_t old_end = allocator->end_address;
        uintptr_t new_end = contract((uintptr_t)block, allocator);
        size_t space_removed = old_end - new_end;

        /* the node will still exist, but needs to be smaller */
        if (block_size - space_removed > 0) {
            set_size(block, block_size - space_removed);
            mark_free(block);
            footer = get_footer(block);
            set_magic(footer);
            footer->header = block;
        }
        else {
            /* the node must be removed so don't insert it */
            insert = 0;
        }
    }
    
    if (insert && !insert_rbnode(&allocator->mem_tree, block_to_rbnode(block), (void *)0)) {
        kprintf(ERROR, "\033\014Error: can't insert node into RB-tree\n\033\017");
    }
}

allocator_t *create_mem_allocator(uintptr_t start, uintptr_t end, size_t min_size, size_t max_size, 
                                  uint8_t supervisor, uint8_t readonly, struct page_dir *dir)
{
    if (start % FRAME_SIZE || end % FRAME_SIZE) {
        kprintf(ERROR, "\033\014starting and ending heap addresses must be page-aligned!\n\033\017");
        return (allocator_t *)0;
    }

    allocator_t *allocator = (allocator_t *)start;

    start += sizeof(allocator_t);
    size_t granularity = MIN_BLOCK_SIZE;
    if (start % granularity) {
        start -= (start % granularity);
        start += granularity;
    }

    init_rbtree(&allocator->mem_tree, compare, select_dup);
    allocator->mem_used = 0;
    allocator->start_address = start;
    allocator->end_address = end;
    allocator->min_size = min_size;
    allocator->max_size = max_size;
    allocator->page_dir = dir;
    allocator->supervisor = supervisor;
    allocator->readonly = readonly;

    alloc_header_t *hole = (alloc_header_t *)start;
    set_size(hole, end - start);
    set_magic(hole);
    alloc_footer_t *footer = get_footer(hole);
    footer->header = hole;
    set_magic(footer);
    mark_free(hole);

    insert_rbnode(&allocator->mem_tree, block_to_rbnode(hole), (void *)0);

    return allocator;
}

size_t mem_used(allocator_t *allocator)
{
    return allocator->mem_used;
}

size_t mem_free(allocator_t *allocator)
{
    return allocator->end_address - allocator->start_address - allocator->mem_used;
}
