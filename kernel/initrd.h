#ifndef __KERNEL_INITRD_H__
#define __KERNEL_INITRD_H__

#include <types.h>
#include <vfs.h>

#define INITRD_MAGIC 0xbf

typedef struct
{
    uint32_t nfiles;
} initrd_header_t;

typedef struct
{
    uint8_t magic;
    int8_t name[128];
    uint32_t offset;
    uint32_t length;
} initrd_file_header_t;

vnode_t *initrd_init(uintptr_t location);

#endif
