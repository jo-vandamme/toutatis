#ifndef __KERNEL_VFS_H__
#define __KERNEL_VFS_H__

#include <types.h>
#include <vnode.h>

/* The virtual file system is an abstraction that should allow
 * the user to access files/directories/pipes/sockets/screen/keyboard/mouse....
 * transparently.
 * It creates an abstraction on top of the concrete file systems.
 */

struct statfs;

typedef struct vfs
{
    const char   *name;
    struct vfs   *next;         /* next VFS in list */
    struct vfs   *prev;         /* prev VFS in list */
    struct vnode *vnodecovered; /* mountpoint */
    void         *data;         /* implementation-specific data */
    int          (*init)();
    int          (*done)();
    int          (*start)(struct vfs *vfs, int flags);
    int          (*mount)(struct vfs *vfs, char *path, void *data);
    int          (*unmount)(struct vfs *vfs);
    int          (*root)(struct vfs *vfs, struct vnode **vpp);
    int          (*sync)(struct vfs *vfs);
    int          (*statfs)(struct vfs *vfs, struct statfs *stat);
} vfs_t;

#endif
