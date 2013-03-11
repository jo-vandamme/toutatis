#ifndef __KERNEL_DRIVER_H__
#define __KERNEL_DRIVER_H__

#include <types.h>

typedef struct device_s
{
        size_t (*read) (u8_t *data, size_t len);
        size_t (*write)(u8_t *data, size_t len);
} device_t;

typedef device_t *(driver_init)(void);

#endif
