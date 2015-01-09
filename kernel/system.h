#ifndef __KERNEL_SYSTEM_H__
#define __KERNEL_SYSTEM_H__

#include <types.h>

#define asm __asm__
#define volatile __volatile__
//#define asm __asm__ __volatile__

#define IRQ(x)          ((x) + 0x20)
#define SYSCALL_VECTOR  0x80
#define TIMER_FREQ      1000
#define IRQ_TIMER       0

#define min(x, y)       ((x) < (y) ? (x) : (y))

#define _DEBUG_
#ifdef _DEBUG_
    #define DBPRINT(...)    do { kprintf(DEBUG, __VA_ARGS__); } while (0)
#else
    #define DBPRINT(...)    do {} while (0)
#endif

typedef uint32_t irq_state_t;

/*
inline static void print1(void)
{
    uint16_t *video = (uint16_t *)(0xc00b8000 + 79*2);
    *video = (uint16_t)'i' | 0x4f00; // red bg
}

inline static void print2(void)
{
    uint16_t *video = (uint16_t *)(0xc00b8000 + 79*2);
    *video = (uint16_t)'i' | 0x2f00; // green bg 
}

inline static void print_debug(uint8_t c, uint8_t att, uint8_t row, uint8_t col)
{
    uint16_t *video = (uint16_t *)(0xc00b8000 + (row * 80 + col)*2);
    *video = (uint16_t)c | (uint16_t)(att << 8); // cyan bg
}
*/

inline static void irq_disable(void)
{
    asm volatile("cli");
    //print1();
}

inline static void irq_enable(void)
{
    asm volatile("sti");
    //print2();
}

inline static irq_state_t irq_save(void)
{
    irq_state_t state;
    asm volatile("pushfl; popl %0; cli" : "=rm" (state));
    //print1();
    return state;
}

inline static void irq_restore(irq_state_t state)
{
    asm volatile("pushl %0; popfl" : : "rm" (state));
    /*
    if (state & (1 << 9)) {
        print2();
    } else {    
        print1();
    } */
}

inline static void halt()
{
    asm volatile("hlt");
}

inline static void stop()
{
    while (1) { halt(); }
}

#define assert(x) { \
    if (!(x)) { \
        irq_disable();  \
        kprintf(CRITICAL, "\033\014Assertion failed: %s, at %s:%d (%s)\n", #x, \
                __FILE__, __LINE__, __PRETTY_FUNCTION__); \
        stop(); \
    } \
}

// TODO: implement mutex like locklessinc.com/articles/locks/
void spin_lock(uint8_t volatile *lock);
void spin_unlock(uint8_t volatile *lock);

void gdt_flush(void *pointer); /* XXX: should be in gdt.h */
void idt_flush(void *pointer); /* XXX: should be in idt.h */
void tss_flush(); /* XXX: should be in tss.h */
void set_kernel_stack(uintptr_t stack); /* XXX: should be in tss.h */

typedef struct registers
{
    uint32_t gs, fs, es, ds;
    uint32_t ebp, edi, esi, edx, ecx, ebx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, esp, ss;
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
void arch_reset();

void enable_irq(uint8_t irq);
void disable_irq(uint8_t irq);
void disable_irqs();
void restore_irqs();

void attach_interrupt_handler(uint8_t num, isr_t handler);
void detach_interrupt_handler(uint8_t num, isr_t handler);
handler_t *get_interrupt_handler(uint8_t num);
void interrupt(int no);

uint32_t get_ticks_count();
uint64_t get_cycles_count();

void cpuid(int code, uint32_t *a, uint32_t *d);

void sleep(uint32_t ms);
void io_wait();

void outb(uint16_t port, uint8_t data);
void outw(uint16_t port, uint16_t data);
void outl(uint16_t port, uint32_t data);

uint8_t  inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

//void copy_frame(uintptr_t src, uintptr_t dest);

#endif
