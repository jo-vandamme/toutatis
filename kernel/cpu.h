#ifndef __KERNEL_CPU_H__
#define __KERNEL_CPU_H__

#include <types.h>

void *cpu_get_gdt();
void  cpu_set_gdt(void *pointer);

void *cpu_get_idt();
void  cpu_set_idt(void *pointer);

/* set/get page directory base register (cr3) */
void cpu_set_pdbr(u32_t addr);
u32_t cpu_get_pdbr();
void cpu_enable_paging(int enable);
void cpu_flush_tlb_entry(u32_t addr);

#endif
