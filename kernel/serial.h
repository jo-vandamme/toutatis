#ifndef __KERNEL_SERIAL_H__
#define __KERNEL_SERIAL_H__

#include <driver.h>
#include <types.h>

device_t *serial_init(void);
void serial_terminate(void);
size_t write(u8_t *data, size_t len);

#endif
