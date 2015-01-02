#include <system.h>
#include <gdt.h>
#include <idt.h>
#include <pic.h>
#include <pit.h>
#include <logging.h>
#include <process.h>

#define MAX_HANDLERS 50

static handler_t handlers[IDT_NUM_ENTRIES][MAX_HANDLERS];
static uint32_t handlers_heads[IDT_NUM_ENTRIES] = { 0 };

static void dump_registers(registers_t *regs);
extern char *exception_messages[];

extern int multitasking;
extern tss_entry_t tss_entry;

inline void spin_lock(uint8_t volatile *lock) {
    while (__sync_lock_test_and_set(lock, 0x01)) {
    }
}

inline void spin_unlock(uint8_t volatile *lock) {
    __sync_lock_release(lock);
}

void set_kernel_stack(uintptr_t stack)
{
    tss_entry.esp0 = (uint32_t)stack; }

void arch_init()
{
    gdt_init();
    idt_init();
    kprintf(INFO, "[system] GDT/IDT initialized\n");

    pic_init();
    kprintf(INFO, "[system] PIC initialized\n");

    pit_init(TIMER_FREQ);
    kprintf(INFO, "[system] PIT initialized\n");
}

void arch_finish()
{
    pic_disable();
}

void arch_reset()
{
    cli();

    /* flush the keyboard buffers (output and command) */
    uint8_t temp;
    do
    {
        temp = inb(0x64); /* empty user data */
        if ((temp & 0x01) != 0)
            (void)inb(0x60); /* empty keyboard data */
    } while ((temp & 0x02) != 0);

    /* pulse cpu reset line */
    outw((short)0x64, (short)0xfe);

    stop();
}

void attach_interrupt_handler(uint8_t num, isr_t handler)
{
    handler_t *h = 0, *n = 0;
    uint8_t i = 0;
    uint32_t head;

    /* get the head */
    head = handlers_heads[num];
    h = &handlers[num][head];

    /* null h if the list is empty */
    if (!h->handler) {
        h = 0;
    }

    /* find the tail */
    while (h && h->next) {
        h = h->next;
    }

    /* h now points to the last interrupt handler structure
     * or is 0 if none has been registered yet */

    /* find a free slot */
    for (i = 0; i < MAX_HANDLERS; ++i) {
        if (handlers[num][i].handler == 0) {
            n = &handlers[num][i];
            break;
        }
    }
    /* attach handler if a slot is available */
    if (n) {
        cli();
        n->handler = handler;
        n->num = num;
        n->next = 0;
        if (h) {
            n->index = h->index + 1;
            n->prev = h;
            n->prev->next = n;
        } else {
            n->index = 1;
            n->prev = 0;
        }
        sti();
    }
}

void detach_interrupt_handler(uint8_t num, isr_t handler)
{
    /* get the head */
    uint32_t head = handlers_heads[num];
    handler_t *h = &handlers[num][head];

    /* check for empty list */
    if (!h->handler) {
        return;
    }

    /* go to the tail */
    while (h->next) {
        h = h->next;
    }

    /* go through the list from tail to head and look for handler */
    do {
        if (h->handler == handler) {
            cli();
            if (h->prev) {
                h->prev->next = h->next;
            }
            if (h->next) {
                h->next->prev = h->prev;
                /* If we are removing the head we need
                 * to update handlers_heads accordingly. */
                if (!h->prev) {
                    /* Since the array is static it is guaranteed
                     * to be contiguous */
                    handlers_heads[num] = (uint32_t)(h->next - &handlers[num][0]);
                    /* if h->next is 0, the list is empty and head will be 0 */
                }
            }
            h->handler = 0;
            sti();
            break;
        }
    } while ((h = h->prev) != 0);
}

handler_t *get_interrupt_handler(uint8_t num)
{
    uint32_t head = handlers_heads[num];
    handler_t *h = &handlers[num][head];

    if (h->handler) {
        return h;
    } else {
        return 0;
    }
}

inline void enable_irq(uint8_t irq)
{
    pic_enable_irq(irq);
}

inline void disable_irq(uint8_t irq)
{
    pic_disable_irq(irq);
}

inline void disable_irqs()
{
    pic_disable();
}

inline void restore_irqs()
{
    //pic_restore();
}

inline void sleep(uint32_t ms)
{
    uint32_t current = pit_get_ticks();

    while (current + ms > pit_get_ticks()) ;
}

inline uint32_t get_ticks_count()
{
    return pit_get_ticks();
}

inline uint64_t get_cycles_count()
{
    uint64_t ret;
    asm volatile ("rdtsc" : "=A"(ret));
    return ret;
}

void cpuid(int code, uint32_t *a, uint32_t *d)
{
    asm volatile ("cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "ebx", "ecx");
}

uintptr_t isr_handler(registers_t *regs)
{
    uintptr_t esp = (uintptr_t)regs;
    uint8_t stop = 0;

    handler_t *h = get_interrupt_handler(regs->int_no);

    if (!h /* no handler */
        || regs->int_no == 8 /* double fault */
        || (regs->int_no >= IRQ(0) && regs->int_no != SYSCALL_VECTOR)) {/* unexpected int. no */
        stop = 1;
    }

    while (h) {
        h->handler(regs);
        h = h->next;
    }

    if (stop) {
        kprintf(ERROR, "\033\014Unhandled exception #%u", regs->int_no);
        if (regs->int_no < IRQ(0)) {
            kprintf(ERROR, " (%s)", exception_messages[regs->int_no]);
        }
        kprintf(ERROR, "\n");
        dump_registers(regs);
        for (;;) halt();
    }

    return esp;
}

uintptr_t irq_handler(registers_t *regs)
{
    uintptr_t esp = (uintptr_t)regs;
    handler_t *h = 0;

    if (pic_acknowledge(regs->int_no)) {
        kprintf(DEBUG, "Spurious IRQ");
        return esp; /* ignore spurious IRQs */
    }

    if (multitasking && regs->int_no == 0) {
        /* execute scheduler and overwrite esp */
        esp = switch_tasks(regs);
    }

    h = get_interrupt_handler(IRQ(regs->int_no));
    if (!h && IRQ(regs->int_no) != 0) { /* XXX: IRQ ?? why */
        kprintf(WARNING, "No handler for IRQ #%u", regs->int_no);
        return esp;
    }
    while (h) {
        h->handler(regs);
        h = h->next;
    }

    return esp;
}

static void dump_registers(registers_t *regs)
{
    kprintf(ERROR,
            "eax: %#010x ebx: %#010x\n"
            "ecx: %#010x edx: %#010x\n"
            "esi: %#010x edi: %#010x\n"
            "ebp: %#010x esp: %#010x\n"
            "eip: %#010x efl: %#010x\n"
            "ss: %#04x cs: %#04x ds: %#04x\n"
            "es: %#04x fs: %#04x gs: %#04x\n\033\017",
            regs->eax, regs->ebx, regs->ecx, regs->edx,
            regs->esi, regs->edi, regs->ebp, regs->esp,
            regs->eip, regs->eflags,
            regs->ss, regs->cs, regs->ds,
            regs->es, regs->fs, regs->gs);
}

char *exception_messages[] =
{
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Detected overflow",
    "Out-of-bounds",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unknown interrupt",
    "Coprocessor fault",
    "Alignment check (486+)",
    "Machine check (Pentium/586+)",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};
