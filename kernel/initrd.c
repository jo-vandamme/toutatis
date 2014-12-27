#include <initrd.h>
#include <vfs.h>
#include <kheap.h>
#include <logging.h>

vnode_t *initrd_init(uintptr_t location)
{
    initrd_header_t *header = (initrd_header_t *)location;
    initrd_file_header_t *file_headers = 
        (initrd_file_header_t *)(location + sizeof(initrd_header_t));

    kprintf(INFO, "Initrd contains %d files\n", header->nfiles);
    (void)file_headers;

    vnode_t *node = (vnode_t *)kmalloc(sizeof(vnode_t)); 


    //vnode_init(node, const struct vnode_ops *ops, vfs_t *vfs, void *data)
    return node;
}
