#ifndef __KERNEL_PIT_H__
#define __KERNEL_PIT_H__

#include <types.h>

void pit_init(uint32_t freq);
uint32_t pit_get_ticks();
void pit_set_ticks(uint32_t val);

#endif
