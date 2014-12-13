#ifndef __KERNEL_SYSTEM_H__
#define __KERNEL_SYSTEM_H__

#include <types.h>

#define asm __asm__
#define volatile __volatile__

#define IRQ(x)          ((x) + 0x20)
#define SYSCALL_VECTOR  0x7f
#define TIMER_FREQ      100

#define INT_OFF { asm volatile ("cli"); }
#define INT_ON  { asm volatile ("sti"); }
#define HALT    { asm volatile ("hlt"); }
#define STOP    while (1) { HALT; }

void gdt_flush(void *pointer);
void idt_flush(void *pointer);

typedef struct
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef void (*isr_t)(registers_t *);
typedef struct handler_s
{
    isr_t handler;
    uint8_t num;
    uint32_t index;
    struct handler_s *next, *prev;
} handler_t;

void arch_init();
void arch_finish();

void enable_irq(uint8_t irq);
void disable_irq(uint8_t irq);
void disable_irqs();
void restore_irqs();

void attach_interrupt_handler(uint8_t num, isr_t handler);
void detach_interrupt_handler(uint8_t num, isr_t handler);
handler_t *get_interrupt_handler(uint8_t num);
void interrupt(int no);

uint32_t get_ticks_count();

void sleep(uint32_t ms);
void io_wait();

void outb(uint16_t port, uint8_t data);
void outw(uint16_t port, uint16_t data);
void outl(uint16_t port, uint32_t data);

uint8_t  inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

#endif
