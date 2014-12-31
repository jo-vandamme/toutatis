#include <vnode.h>
#include <logging.h>
#include <system.h>
#include <kheap.h>

static uint32_t freelist_size = 0;
static vnode_t *freelist_head = NULL;

static vnode_t *vnode_get()
{
    vnode_t *node;
    if (freelist_size > 0) {
        node = freelist_head; 
        freelist_head = freelist_head->freelist;
        --freelist_size;
    }
    else {
        node = (vnode_t *)kmalloc(sizeof(vnode_t));
    }
    return node;
}

vnode_t *vnode_init(const struct vn_ops *ops, struct vfs *vfs, void *data)
{
    assert(ops != NULL);
    assert(vfs != NULL);

    vnode_t *node = vnode_get();
    node->ref_count = 1;
    node->vfs = vfs;
    node->ops = ops;
    node->data = data;
    node->freelist = NULL;

    return node;
}

void vnode_kill(vnode_t *node)
{
    assert(node != NULL);
    assert(node->ref_count == 1);

    if (freelist_size < VNODE_CACHE_SIZE) {
        node->freelist = freelist_head;
        freelist_head = node;
        ++freelist_size;
    }
    else {
        kfree(node);
    }
}

void vnode_incref(vnode_t *node)
{
    assert(node != NULL);
    ++node->ref_count;
}

void vnode_decref(vnode_t *node)
{
    assert(node != NULL);
    assert(node->ref_count > 0);

    int release = 0;
    if (node->ref_count > 1) {
        --node->ref_count;
    }
    else {
        release = 1;
    }

    if (release) {
        if (VOP_RECLAIM(node)) {
            kprintf(WARNING, "[vfs] VOP_RECLAIM error\n");
        }
        VN_KILL(node);
    }
}

inline void vnode_check(vnode_t *node, const char *name, uintptr_t address)
{
    if (!node) {
        kprintf(CRITICAL, "Null vnode\n");
        stop();
    }
    if (!address) {
        kprintf(CRITICAL, "No operation for %s\n", name);
        stop();
    }
}

