#ifndef __KERNEL_ARCH_H__
#define __KERNEL_ARCH_H__

#include <types.h>

#define IRQ(x)          ((x) + 0x20)
#define SYSCALL_VECTOR  0x80
#define TIMER_FREQ      1000

typedef struct
{
        u32_t gs, fs, es, ds;
        u32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
        u32_t int_no, err_code;
        u32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef void (*isr_t)(registers_t *);
typedef struct handler_s
{
        isr_t handler;
        u8_t num;
        u32_t index;
        struct handler_s *next, *prev;
} handler_t;

void arch_init();
void arch_finish();

void enable_irq(u8_t irq);
void disable_irq(u8_t irq);
void disable_irqs();
void restore_irqs();

void attach_interrupt_handler(u8_t num, isr_t handler);
void detach_interrupt_handler(u8_t num, isr_t handler);
handler_t *get_interrupt_handler(u8_t num);
void interrupt(int no);

u32_t get_ticks_count();

void sleep(u32_t ms);
void halt();
void cli();
void sti();
void io_wait();

void outb(u16_t port, u8_t data);
void oubw(u16_t port, u16_t data);
void outl(u16_t port, u32_t data);

u8_t  inb(u16_t port);
u16_t inw(u16_t port);
u32_t inl(u16_t port);

#endif
