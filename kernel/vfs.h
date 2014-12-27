#ifndef __KERNEL_VFS_H__
#define __KERNEL_VFS_H__

#include <types.h>
#include <vnode.h>

/* The virtual file system is an abstraction that should allow
 * the user to access files/directories/pipes/sockets/screen/keyboard/mouse....
 * transparently.
 * It creates an abstraction on top of the concrete file systems.
 */

typedef struct vfs
{
    struct vfs   *next;         /* next VFS in list */
    struct vnode *vnode;        /* mountpoint */
    void         *data;         /* implementation-specific data */
    int          (*mount)();
    int          (*unmount)();
    int          (*root)();
    int          (*sync)();
    int          (*stat)();
} vfs_t;

#endif
