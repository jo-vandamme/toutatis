#include <paging.h>
#include <logging.h>
#include <vsprintf.h>
#include <kheap.h>

#define get_size(node)              ((node)->size & 0xfffffffc)
#define get_flags(node)             ((node)->size & 0x00000003)
#define is_free(node)               ((node) && (node)->size & (1 << 0))
#define is_red(node)                ((node) && (node)->size & (1 << 1))

#define mark_free(node)             ((node)->size |=  (1 << 0))
#define mark_used(node)             ((node)->size &= ~(1 << 0))
#define paint_red(node)             ((node)->size |=  (1 << 1))
#define paint_black(node)           ((node)->size &= ~(1 << 1))

#define MIN_BLOCK_SIZE              rounded_size(sizeof(header_t) + sizeof(footer_t))
#define USER_PTR_OFFSET             (2 * sizeof(uint32_t))

#define user_ptr(node)              ((void *)((uintptr_t)(node) + USER_PTR_OFFSET))
#define node_ptr(ptr)               ((header_t *)((uintptr_t)(ptr) - USER_PTR_OFFSET))
#define get_footer(node)            ((footer_t *)((uintptr_t)(node) + get_size(node) - sizeof(footer_t)))
#define get_next_node(node)         ((header_t *)((uintptr_t)(node) + get_size(node)))
#define get_real_size(size)         (rounded_block_size(size + USER_PTR_OFFSET + sizeof(footer_t))) 

extern uint32_t kernel_end;
uintptr_t placement_address = (uintptr_t)&kernel_end;
extern page_dir_t *kernel_directory; 
heap_t *kheap = 0;

static uintptr_t kmalloc_int(uint32_t size, uint32_t alignment, uintptr_t *phys)
{
    if (kheap != 0) {
        void *addr = alloc(size, alignment, kheap);
        if (phys != 0) {
            pte_t *page = get_page((uintptr_t)addr, 0, kernel_directory);
            *phys = page->frame * FRAME_SIZE + ((uintptr_t)addr & 0xfff);
        }
        return (uintptr_t)addr;
    }
    else {
        if (alignment != 0 && (placement_address % alignment)) {
            placement_address -= (placement_address % alignment);
            placement_address += alignment;
        }
        if (phys) {
            *phys = placement_address;
        }
        uintptr_t tmp = placement_address;
        placement_address += size;
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
    free(p, kheap);
}

static inline uint32_t rounded_size(uint32_t size)
{
    return ((size + sizeof(long) - 1) / sizeof(long)) * sizeof(long);
}

static inline uint32_t rounded_block_size(uint32_t size)
{
    uint32_t s = rounded_size(size);
    return s < MIN_BLOCK_SIZE ? MIN_BLOCK_SIZE : s;
}

static inline header_t *rotation(header_t *root, int dir)
{
    header_t *save = root->link[!dir];

    root->link[!dir] = save->link[dir];
    save->link[dir] = root;

    paint_red(root);
    paint_black(save);

    return save;
}

static int insert_node(tree_t *tree, header_t *node)
{
    node->size = get_size(node);
    node->link[2] = node->link[1] = node->link[0] = NULL;
    paint_red(node);
    mark_free(node);

    if (tree->root == NULL) {
        /* empty tree case */
        tree->root = node;
        if (tree->root == NULL) {
            return 0;
        }
        tree->num_nodes = 1;
    }
    else {
        header_t head = { 0 }; /* dummy tree root */

        header_t *g, *t; /* grandparent and grandgrandparent */
        header_t *p, *n; /* parent and current node */
        int dir = 0, last = 0;
        int duplicate = 1;

        /* set up helpers */
        t = &head;
        g = p = NULL;
        n = t->link[1] = tree->root;

        /* search down the tree */
        for (;;) {
            if (n == NULL) {
                /* insert new node at the bottom */
                p->link[dir] = n = node;

                if (n == NULL) {
                    return 0;
                }
                duplicate = 0;
                ++tree->num_nodes;
            }
            /* simple red violation - make a color flip */
            else if (is_red(n->link[0]) && is_red(n->link[1])) {
                paint_red(n);
                paint_black(n->link[0]);
                paint_black(n->link[1]);
            }

            /* hard red violation: make rotations */
            if (is_red(n) && is_red(p)) {
                int dir2 = t->link[1] == g;

                if (n == p->link[last]) {
                    t->link[dir2] = rotation(g, !last);
                } else {
                    g->link[last] = rotation(g->link[last], last);
                    t->link[dir2] = rotation(g, !last);
                }
            }

            /* stop working if we inserted a node */
            if (get_size(n) == get_size(node)) {
                /* prepend a duplicate if necessary */
                if (duplicate) {// && n != node) {
                    node->link[2] = n->link[2];
                    n->link[2] = node;
                    ++tree->num_duplicates;
                }
                break;
            }

            last = dir;
            dir = get_size(n) < get_size(node);

            /* update helpers */
            t = (g != NULL) ? g : t;
            g = p, p = n;
            n = n->link[dir];
        }

        /* update root */
        tree->root = head.link[1];
    }

    /* make root black */
    paint_black(tree->root);

    return 1;
}

/* Searches and removes a block of size "size", if the node has duplicates,
 * the first block of the list, after the node, is removed.
 * If an address is given, only a block of size "size" with a matching address
 * will be removed.
 */
static header_t *remove_node(tree_t *tree, size_t size, int best_fit, uintptr_t address, uint32_t alignment)
{
    if (best_fit && address) {
        best_fit = 0;
    }
    if (tree->root != NULL) {
        header_t head = { 0 };   /* dummy tree root */
        header_t *n, *p, *g;     /* helpers */
        header_t *f = NULL, *fp = NULL; /* found node and parent */
        int dir = 1, fdir = 1, duplicate = 0;

        /* set up helpers */
        n = &head;
        g = p = NULL;
        n->link[1] = tree->root;

        /* search and push a red down to fix red violations as we go */
        while (n->link[dir] != NULL) {
            int last = dir;

            /* move helpers down */
            g = p, p = n;
            n = n->link[dir];
            dir = get_size(n) < size; /* find in-order predecessor */

            /* if alignment is required, calculate the size so that user_ptr(n) is
             * aligned and we have enough space to create a block with the orphaned
             * space needed for alignment */
            int align_flag = alignment == 0;
            if (alignment && get_size(n) >= size) {
                uint32_t offset = 0;
                if (((uint32_t)n + 2 * sizeof(uint32_t)) % alignment) {
                    offset = alignment - ((uint32_t)n + 2 * sizeof(uint32_t)) % alignment;
                    while (offset < MIN_BLOCK_SIZE) {
                        offset += alignment;
                    }
                }
                if (get_size(n) - offset >= size) {
                    align_flag = 1;
                }
            }
            if (align_flag && (get_size(n) == size || (best_fit && get_size(n) > size))) {
                /* save found node and keep going; we'll do the removal at the end */
                f = n, fp = p, fdir = last;
                duplicate = n->link[2] != NULL;
                /* if the node has duplicates and has the required size, we
                 * don't need to continue traversing the tree to find a 
                 * predecessor for replacing the node */
                if (duplicate && get_size(n) == size) {
                    break;
                }
            }

            /* push the red node down with rotations and color flips */
            if (!is_red(n) && !is_red(n->link[dir])) {
                if (is_red(n->link[!dir])) {
                    p = p->link[last] = rotation(n, dir);
                }
                else { //if (!is_red(n->link[!dir])) {
                    header_t *s = p->link[!last]; // always black because of the previous step

                    if (s != NULL) {
                        if (!is_red(s->link[0]) && !is_red(s->link[1])) {
                            /* color flip */
                            paint_black(p);
                            paint_red(s);
                            paint_red(n);
                        }
                        else {
                            int dir2 = g->link[1] == p;

                            if (is_red(s->link[last])) {
                                p->link[!last] = rotation(p->link[!last], !last);
                                g->link[dir2] = rotation(p, last);
                            }
                            else if (is_red(s->link[!last])) {
                                g->link[dir2] = rotation(p, last);
                            }

                            /* ensure correct coloring */
                            paint_red(n);
                            paint_red(g->link[dir2]);
                            paint_black(g->link[dir2]->link[0]);
                            paint_black(g->link[dir2]->link[1]);
                        }
                    }
                }
            }
        }
        /* remove a duplicate (matching address if necessary) */
        if (duplicate) {
            /* replace tree node by its first duplicate in the list */
            if (address && (uintptr_t)f == address) {
                fp->link[fdir] = f->link[2];
                fp->link[fdir]->link[0] = f->link[0];
                fp->link[fdir]->link[1] = f->link[1];
                --tree->num_duplicates;
            } else {
                /* iterate over the duplicate list to find a node
                 * matching the requested address, if no match is found
                 * f will be null, if no address is requested,
                 * f will point to the first element of the list, after
                 * the tree node */
                header_t *prev = f;
                f = f->link[2];
                while (address && (uintptr_t)f != address && f != NULL) {
                    prev = f;
                    f = f->link[2];
                }
                /* remove duplicate */
                if (f != NULL) {
                    prev->link[2] = f->link[2];
                    --tree->num_duplicates;
                }
            }
        }
        /* replace and remove tree node if found */
        else if (f != NULL) {
            /* remove inorder predecessor from tree */
            p->link[p->link[1] == n] = n->link[n->link[0] == NULL];
            --tree->num_nodes;

            /* replace found node by inorder predecessor 
             * if they are not the same node */
            if (n != f) {
                n->link[0] = f->link[0];
                n->link[1] = f->link[1];
                n->size = get_size(n) | get_flags(f);
                fp->link[fdir] = n;
            }
        }

        /* update root and make it black */
        tree->root = head.link[1];
        if (tree->root != NULL) {
            paint_black(tree->root);
        }

        if (f != NULL) mark_used(f);
        return f;
    }

    return NULL;
}

static void expand(uint32_t new_size, heap_t *heap)
{
    if (new_size <= heap->end_address - heap->start_address) {
        kprintf(ERROR, "Heap expansion must be bigger than actual size!\n");
        return;
    }

    if (new_size % FRAME_SIZE) {
        new_size -= (new_size % FRAME_SIZE);
        new_size += FRAME_SIZE;
    }

    if (heap->start_address + new_size > heap->max_address) {
        kprintf(ERROR, "Heap expansion overflow!\n");
        return;
    }

    /* allocate some pages */
    uint32_t i = heap->end_address - heap->start_address; /* old size */
    while (i < new_size) {
        alloc_page(get_page(heap->start_address + i, 1, kernel_directory),
                (heap->supervisor) ? 1 : 0, (heap->readonly) ? 0 : 1);
        i += FRAME_SIZE;
    }
    heap->end_address = heap->start_address + new_size;
}

static uint32_t contract(uint32_t new_size, heap_t *heap)
{
    if (new_size >= heap->end_address - heap->start_address) {
        kprintf(ERROR, "Heap contraction must be smaller than actual size\n");
        return 0;
    }

    if (new_size % FRAME_SIZE) {
        new_size -= (new_size % FRAME_SIZE);
        new_size += FRAME_SIZE;
    }

    if (new_size < HEAP_MIN_SIZE) {
        new_size = HEAP_MIN_SIZE;
    }

    uint32_t old_size = heap->end_address - heap->start_address;
    uint32_t i = old_size - FRAME_SIZE;
    while (new_size < i) {
        free_page(get_page(heap->start_address + i, 0, kernel_directory));
        i -= FRAME_SIZE;
    }
    heap->end_address = heap->start_address + new_size;

    return new_size;
}

void *alloc(uint32_t size, uint32_t alignment, heap_t *heap)
{
    /* round size to multiple of the length of most restrictive data type */
    uint32_t real_size = get_real_size(size);

    /* get a block of size "size" or bigger */
    header_t *node = remove_node(&heap->tree, real_size, 1, 0, alignment);

    if (!node) {
        /* expand the heap */
        uint32_t old_size = heap->end_address - heap->start_address;
        uint32_t old_end_address = heap->end_address;

        expand(old_size + real_size, heap);
        uint32_t new_size = heap->end_address - heap->start_address;

        /* create a block with the added space */
        header_t *hole = (header_t *)old_end_address;
        hole->size = new_size - old_size;
        hole->magic = MAGIC;
        footer_t *hole_footer = get_footer(hole);
        hole_footer->header = hole;
        hole_footer->magic = MAGIC;

        footer_t *left_footer = (footer_t *)(old_end_address - sizeof(footer_t)); 
        if ((uintptr_t)left_footer->header >= heap->start_address && is_free(left_footer->header)) {
            /* remove block with largest address if free */
            header_t *left_node = remove_node(&heap->tree, get_size(left_footer->header),
                    0, (uintptr_t)left_footer->header, 0);
            if (!left_node) {
                kprintf(ERROR, "ERROR: expansion failed, left footer not found\n");
                return 0;
            }
            /* expand block */
            left_node->size = get_size(left_node) + get_size(hole);
            hole_footer->header = left_node;
            
            /* insert node back into the tree */
            insert_node(&heap->tree, left_node);
        } else {
            insert_node(&heap->tree, hole);
        }
        /* recurse, with a bigger heap this time */
        return alloc(size, alignment, heap);
    }

    /* align block if required */
    if (alignment && ((uint32_t)node + 2 * sizeof(uint32_t)) % alignment) {

        uint32_t offset = alignment - ((uint32_t)node + 2 * sizeof(uint32_t)) % alignment;
        while (offset < MIN_BLOCK_SIZE) {
            offset += alignment;
        }
        header_t *new_node = (header_t *)((uintptr_t)node + offset);
        new_node->size = get_size(node) - offset;

        /* return preceeding block to tree */
        if (offset >= MIN_BLOCK_SIZE) {
            node->magic = MAGIC;
            node->size = offset;
            footer_t *footer = get_footer(node);
            footer->header = node;
            footer->magic = MAGIC;

            insert_node(&heap->tree, node);
        }
        node = new_node;
    }

    /* fragment node if necessary */
    uint32_t block_size = get_size(node);
    int32_t frag_size = block_size - real_size;
    if (frag_size > 0 && (uint32_t)frag_size >= MIN_BLOCK_SIZE) {

        /* set header and footer of user block and mark it as used */
        node->size = real_size;
        node->magic = MAGIC;
        footer_t *footer = get_footer(node);
        footer->header = node;
        footer->magic = MAGIC;
        mark_used(node);

        /* set header and footer of leftover and insert it back in the tree */
        header_t *next_node = get_next_node(node);
        next_node->size = frag_size;
        next_node->magic = MAGIC;
        footer = get_footer(next_node);
        footer->header = next_node;
        footer->magic = MAGIC;

        insert_node(&heap->tree, next_node);
    }

    return user_ptr(node);
}

void free(void *p, heap_t *heap)
{
    if (p == NULL) {
        return;
    }

    /* get node from user pointer */
    header_t *node = node_ptr(p);

    /* don't free a node already freed */
    if (is_free(node)) {
        kprintf(ERROR, "Error: node already free\n");
        return;
    } else {
        mark_free(node);
    }

    if (node->magic != MAGIC || get_footer(node)->magic != MAGIC) {
        kprintf(ERROR, "Error: Wrong magic number, can't free block 0x%x\n", p);
        return;
    }

    /* right neighbour merge */
    header_t *neighbour = get_next_node(node);
    if ((uintptr_t)neighbour < heap->end_address && neighbour->magic == MAGIC && is_free(neighbour)) {
        if (!remove_node(&heap->tree, get_size(neighbour), 0, (uintptr_t)neighbour, 0)) {
            kprintf(ERROR, "Error: Right neighbour not found in RB-tree!\n");
            return;
        }
        node->size = get_size(node) + get_size(neighbour);
        mark_free(node);
        get_footer(node)->header = node;
    }

    /* left neighbour merge */
    footer_t *footer = (footer_t *)((char *)node - sizeof(footer_t));
    if ((uintptr_t)node > heap->start_address
        && footer->magic == MAGIC && footer->header->magic == MAGIC
        && is_free(footer->header)) {
        neighbour = footer->header;
        if (!remove_node(&heap->tree, get_size(neighbour), 0, (uintptr_t)neighbour, 0)) {
            kprintf(ERROR, "Error: Left neighbour not found in RB-tree!\n");
            return;
        }
        neighbour->size = get_size(neighbour) + get_size(node);
        mark_free(neighbour);
        get_footer(neighbour)->header = neighbour;
        node = neighbour;
    }

    uint8_t insert = 1;
    /* if the footer location is the end address, we can contract */
    if ((uintptr_t)get_footer(node) + sizeof(footer_t) == heap->end_address) {
        uint32_t old_length = heap->end_address - heap->start_address;
        uint32_t new_length = contract((uint32_t)((uintptr_t)node - heap->start_address), heap);

        /* check how big the heap will be after resizing */
        if (get_size(node) - (old_length - new_length) > 0) {
            /* the node will still exist, but needs to be smaller */
            node->size = get_size(node) - (old_length - new_length);
            mark_free(node);
            footer = get_footer(node);
            footer->magic = MAGIC;
            footer->header = node;
        }
        else {
            /* the node must be removed so don't insert it */
            insert = 0;
        }
    }
    
    if (insert && !insert_node(&heap->tree, node)) {
        kprintf(ERROR, "Error: can't insert node into RB-tree\n");
    }
}

heap_t *create_heap(uintptr_t start, uintptr_t end, uintptr_t max, uint8_t supervisor, uint8_t readonly)
{
    if (start % FRAME_SIZE || end % FRAME_SIZE) {
        kprintf(ERROR, "starting and ending heap addresses must be page-aligned!\n");
        return (heap_t *)0;
    }

    heap_t *heap = (heap_t *)start;

    start += sizeof(heap_t);
    if (start % FRAME_SIZE) {
        start -= (start % FRAME_SIZE);
        start += FRAME_SIZE;
    }

    heap->tree.root = 0;
    heap->start_address = start;
    heap->end_address = end;
    heap->max_address = max;
    heap->supervisor = supervisor;
    heap->readonly = readonly;

    header_t *hole = (header_t *)start;
    hole->size = end - start;
    hole->magic = MAGIC;
    footer_t *footer = get_footer(hole);
    footer->header = hole;
    footer->magic = MAGIC;

    insert_node(&heap->tree, hole);

    return heap;
}
