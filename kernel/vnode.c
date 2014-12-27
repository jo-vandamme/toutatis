#include <vnode.h>
#include <logging.h>
#include <system.h>

int vnode_init(vnode_t *node, const struct vn_ops *ops, struct vfs *vfs, void *data)
{
    assert(node != NULL);
    assert(ops != NULL);

    node->ref_count = 1;
    node->vfs = vfs;
    node->ops = ops;
    node->data = data;

    return 0;
}

void vnode_kill(vnode_t *node)
{
    assert(node != NULL);
    assert(node->ref_count == 1);

    node->ref_count = 0;
    node->vfs = NULL;
    node->ops = NULL;
    node->data = NULL;
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
        if (VN_RECLAIM(node)) {
            kprintf(WARNING, "VFS: VOP_RECLAIM error\n");
        }
    }
}

inline void vnode_check(vnode_t *node, const char *name, uintptr_t address)
{
    if (!node) {
        kprintf(CRITICAL, "Null vnode\n");
        STOP;
    }
    if (!address) {
        kprintf(CRITICAL, "No operation for %s\n", name);
        STOP;
    }
}

