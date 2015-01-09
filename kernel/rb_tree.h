#ifndef __KERNEL_RB_TREE_H__
#define __KERNEL_RB_TREE_H__

#include <types.h>

/* Red black tree implementation that supports duplicates
 * red-black information is stored as the last bit of link[0]
 * since pointers are aligned on a 4-byte boundary */

typedef struct rb_node
{
    void *data;
    struct rb_node *link[3];
} rb_node_t;

typedef int (*compare_t)(const rb_node_t *node, const void *data, const void *args);
typedef int (*select_dup_t)(const rb_node_t *node, const void *args);

typedef struct rb_tree
{
    rb_node_t *root;
    uint32_t num_nodes;
    uint32_t num_dup;
    compare_t compare;
    select_dup_t select_dup;
} rb_tree_t;

/* XXX: make a create_rbtree function */
void init_rbtree(rb_tree_t *tree, compare_t, select_dup_t);
int insert_node(rb_tree_t *tree, rb_node_t *node, const void *args);
rb_node_t *remove_node(rb_tree_t *tree, const void *data, const void *args);

void print_tree(rb_tree_t *tree);

#endif 
