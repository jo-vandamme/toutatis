#ifndef __KERNEL_CPU_H__
#define __KERNEL_CPU_H__

void *cpu_get_gdt();
void  cpu_set_gdt(void *pointer);

void *cpu_get_idt();
void  cpu_set_idt(void *pointer);

#endif
