#include <rb_tree.h>
#include <logging.h>

#define is_red(node)            ((node) && ((uintptr_t)(node)->link[0] & (1 << 0)))
#define paint_red(node)         ((node)->link[0] = (rb_node_t *)((uintptr_t)(node)->link[0] |  (1 << 0)))
#define paint_black(node)       ((node)->link[0] = (rb_node_t *)((uintptr_t)(node)->link[0] & ~(1 << 0)))
#define get_link(node, i)       ((rb_node_t *)((uintptr_t)(node)->link[(i)] & ~(1 << 0)))
#define set_link(node, i, val)  ((node)->link[(i)] = (rb_node_t *)(((uintptr_t)(node)->link[(i)] & (1 << 0)) \
                                                                           | ((uintptr_t)val & ~(1 << 0))))
/*
static inline int is_red(rb_node_t *node)
{
    return node && ((uintptr_t)node->link[0] & (1 << 0));
}
static inline void paint_red(rb_node_t *node)
{
    node->link[0] = (rb_node_t *)((uintptr_t)node->link[0] | (1 << 0));
}
static inline void paint_black(rb_node_t *node)
{
    node->link[0] = (rb_node_t *)((uintptr_t)node->link[0] & ~(1 << 0));
}
static inline rb_node_t *get_link(rb_node_t *node, int i)
{
    if (!node) return 0;
    return (rb_node_t *)((uintptr_t)node->link[i] & ~(1 << 0));
}
static inline void set_link(rb_node_t *node, int i, rb_node_t *val)
{
    if (!node) return;
    node->link[i] = (rb_node_t *)(
            ((uintptr_t)node->link[i] & (1 << 0)) | ((uintptr_t)val & ~(1 << 0)));
}*/

static inline rb_node_t *rotate(rb_node_t *root, int dir)
{
    rb_node_t *save = get_link(root, !dir);

    set_link(root, !dir, get_link(save, dir));
    set_link(save, dir, root);
    paint_red(root);
    paint_black(save);

    return save;
}

int insert_node(rb_tree_t *tree, rb_node_t *node, void *args)
{
    if (node == NULL) {
        return 0;
    }
    set_link(node, 0, NULL);
    set_link(node, 1, NULL);
    set_link(node, 2, NULL);
    paint_red(node);

    if (tree->root == NULL) {
        /* empty tree case */
        tree->root = node;
        tree->num_nodes = 1;
        tree->num_dup = 0;
    }
    else {
        rb_node_t head = { 0 }; /* dummy tree root */

        rb_node_t *g, *t; /* grandparent and grandgrandparent */
        rb_node_t *p, *n; /* parent and current node */
        int dir = 0, last = 0;
        int duplicate = 1;

        /* set up helpers */
        t = &head;
        g = p = NULL;
        set_link(t, 1, tree->root);
        n = tree->root;

        /* search down the tree */
        for (;;) {
            if (n == NULL) {
                /* insert new node at the bottom */
                set_link(p, dir, node);
                n = node;

                if (n == NULL) {
                    return 0;
                }
                duplicate = 0;
                ++tree->num_nodes;
            }
            /* simple red violation - make a color flip */
            else if (is_red(get_link(n, 0)) && is_red(get_link(n, 1))) {
                paint_red(n);
                paint_black(get_link(n, 0));
                paint_black(get_link(n, 1));
            }

            /* hard red violation: make rotations */
            if (is_red(n) && is_red(p)) {
                int dir2 = get_link(t, 1) == g;

                if (n == get_link(p, last)) {
                    set_link(t, dir2, rotate(g, !last));
                } else {
                    set_link(g, last, rotate(get_link(g, last), last));
                    set_link(t, dir2, rotate(g, !last));
                }
            }

            /* stop working if we inserted a node */
            if (tree->compare(n, node->data, args) == 0) {
                /* prepend a duplicate if necessary */
                if (duplicate && n != node) {
                    set_link(node, 2, get_link(n, 2));
                    set_link(n, 2, node);
                    ++tree->num_dup;
                }
                break;
            }

            last = dir;
            //dir = get_size(n) < get_size(node);
            dir = tree->compare(n, node->data, args) < 0;

            /* update helpers */
            t = (g != NULL) ? g : t;
            g = p, p = n;
            n = get_link(n, dir);
        }

        /* update root */
        tree->root = get_link(&head, 1);
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

rb_node_t *remove_node(rb_tree_t *tree, void *data, void *args)
{
    if (tree->root != NULL) {
        rb_node_t head = { 0 };   /* dummy tree root */
        rb_node_t *n, *p, *g;     /* helpers */
        rb_node_t *f = NULL, *fp = NULL; /* found node and parent */
        int dir = 1, fdir = 1, duplicate = 0;

        /* set up helpers */
        n = &head;
        g = p = NULL;
        set_link(n, dir, tree->root);

        /* search and push a red down to fix red violations as we go */
        while (get_link(n, dir) != NULL) {
            int last = dir;

            /* move helpers down */
            g = p, p = n;
            n = get_link(n, dir);

            int res = tree->compare(n, data, args);
            dir = res < 0; /* find in-order predecessor */

            if (res == 0) {
                /* save found node and keep going; we'll do the removal at the end */
                f = n, fp = p, fdir = last;
                duplicate = get_link(n, 2) != NULL;
            }

            /* push the red node down with rotations and color flips */
            if (!is_red(n) && !is_red(get_link(n, dir))) {
                if (is_red(get_link(n, !dir))) {
                    set_link(p, last, rotate(n, dir));
                    p = get_link(p, last);

                    /* if the node has been found, its parent may have changed
                     * due to the last rotation */
                    if (f == n) {
                        fp = p, fdir = dir;
                    }
                }
                else {
                    rb_node_t *s = get_link(p, !last); // always black because of the previous step

                    if (s != NULL) {
                        if (!is_red(get_link(s, 0)) && !is_red(get_link(s, 1))) {
                            /* color flip */
                            paint_black(p);
                            paint_red(s);
                            paint_red(n);
                        }
                        else {
                            int dir2 = get_link(g, 1) == p;

                            if (is_red(get_link(s, !last))) {
                                set_link(g, dir2, rotate(p, last));
                            }
                            else if (is_red(get_link(s, last))) {
                                set_link(p, !last, rotate(get_link(p, !last), !last));
                                set_link(g, dir2, rotate(p, last));
                            }

                            /* ensure correct coloring */
                            paint_red(n);
                            paint_red(get_link(g, dir2));
                            paint_black(get_link(get_link(g, dir2), 0));
                            paint_black(get_link(get_link(g, dir2), 1));

                            /* the parent p of n hasn't changed but its grandparent g has, 
                             * so if f (found node) == p, update its parent fp */
                            if (f == p) {
                                fp = get_link(g, dir2), fdir = last;
                            }
                        }
                    }
                }
            }
        }
        /* remove a duplicate (matching address if necessary) */
        if (duplicate) {
            /* try to find a valid duplicate */
            rb_node_t *tmp = get_link(f, 2);
            rb_node_t *tmp_prev = f;
            while (tmp && tree->select_dup(tmp, args) == 0) {
                tmp_prev = tmp;
                tmp = get_link(tmp, 2);
            }
            /* if no valid duplicate was found (tmp = 0), test the first node */
            if (tmp == NULL) {
                if (tree->select_dup(f, args)) {
                    set_link(fp, fdir, get_link(f, 2));
                    set_link(get_link(fp, fdir), 0, get_link(f, 0));
                    set_link(get_link(fp, fdir), 1, get_link(f, 1));
                    --tree->num_dup;
                } else {
                    /* nothing found */
                    f = NULL;
                }
            } else {
                f = tmp;
                set_link(tmp_prev, 2, get_link(tmp, 2));
                --tree->num_dup;
            }
        }
        /* replace and remove tree node if found */
        else if (f != NULL) {
            /* remove inorder predecessor from tree */
            set_link(p, get_link(p, 1) == n, get_link(n, get_link(n, 0) == NULL));
            --tree->num_nodes;

            /* replace found node by inorder predecessor 
             * if they are not the same node */
            if (n != f) {
                set_link(n, 0, get_link(f, 0));
                set_link(n, 1, get_link(f, 1));
                set_link(fp, fdir, n);
                if (is_red(f)) {
                    paint_red(n);
                } else {
                    paint_black(n);
                }
            }
        }
        /* update root and make it black */
        tree->root = get_link(&head, 1);
        if (tree->root != NULL) {
            paint_black(tree->root);
        }
        return f;
    }
    return NULL;
}

void init_rbtree(rb_tree_t *tree, compare_t compare, select_dup_t select)
{
    tree->root = 0;
    tree->compare = compare;
    tree->select_dup = select;
}

/*
#define HEIGHT 20
#define WIDTH  255

static int height(rb_node_t *node)
{
    if (node == NULL) {
        return 0;
    }

    int l = height(get_link(node, 0));
    int r = height(get_link(node, 1));

    if (l > r) {
        return l + 1;
    } else {
        return r + 1;
    }
}

static int print_tree_(rb_node_t *node, int is_left, int offset, int depth, char s[HEIGHT][WIDTH])
{
    char b[20];
    int width = 12;
    int i;

    if (!node) return 0;

    rb_node_t *tmp = get_link(node, 2);
    int dup = 0;
    while (tmp) {
        ++dup;
        tmp = get_link(tmp, 2);
    }
    sprintf(b, "(%5p-%02d-%c)", node->data, dup, is_red(node) ? 'R' : 'B');

    int left  = print_tree_(get_link(node, 0), 1, offset,                depth + 1, s);
    int right = print_tree_(get_link(node, 1), 0, offset + left + width, depth + 1, s);

    for (i = 0; i < width; ++i)
        s[2 * depth][offset + left + i] = b[i];

    if (depth && is_left) {

        for (i = 0; i < width + right; ++i)
            s[2 * depth - 1][offset + left + width/2 + i] = '-';

        s[2 * depth - 1][offset + left + width/2] = '+';
        s[2 * depth - 1][offset + left + width + right + width / 2] = '+';

    } else if (depth && !is_left) {

        for (i = 0; i < left + width; ++i)
            s[2 * depth - 1][offset - width/2 + i] = '-';

        s[2 * depth - 1][offset + left + width/2] = '+';
        s[2 * depth - 1][offset - width/2-1] = '+';
    }

    return left + width + right;
}

void print_tree(rb_tree_t *tree)
{
    char s[HEIGHT][WIDTH];
    int i, j;
    for (i = 0; i < HEIGHT; ++i) {
        for (j = 0; j < WIDTH; ++j) {
            s[i][j] = ' ';
        }
        s[i][j] = '\0';
    }
    print_tree_(tree->root, 0, 0, 0, s);

    int h = height(tree->root);
    for (i = 0; i < h * 2 - 1; ++i) {
        j = WIDTH;
        while (s[i][--j] == ' ') s[i][j] = '\0';
        printf("%s\n", s[i]);
    }
}
*/
void print_tree(rb_tree_t *tree)
{
}
