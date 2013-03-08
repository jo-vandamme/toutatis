#ifndef __KERNEL_PIT_H__
#define __KERNEL_PIT_H__

#include <types.h>

void pit_init(u32_t freq);
u32_t pit_get_ticks();
void pit_set_ticks(u32_t val);

#endif
