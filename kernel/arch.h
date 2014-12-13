#ifndef __KERNEL_ARCH_H__
#define __KERNEL_ARCH_H__

#include <types.h>

#define IRQ(x)          ((x) + 0x20)
#define SYSCALL_VECTOR  0x80
#define TIMER_FREQ      100

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
void halt();
void cli();
void sti();
void io_wait();

void outb(uint16_t port, uint8_t data);
void outw(uint16_t port, uint16_t data);
void outl(uint16_t port, uint32_t data);

uint8_t  inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

#endif
