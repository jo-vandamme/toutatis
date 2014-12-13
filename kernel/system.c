#include <system.h>
#include <gdt.h>
#include <idt.h>
#include <pic.h>
#include <pit.h>
#include <logging.h>

#define ACCESS_KCODE (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW | GDT_ACCESS_EXECUTE)
#define ACCESS_UCODE (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW | GDT_ACCESS_EXECUTE)
#define ACCESS_KDATA (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW)
#define ACCESS_UDATA (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW)
#define GDT_FLAGS    (GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT)
#define IDT_FLAGS_RING0    (IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT)
#define IDT_FLAGS_RING3    (IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_TYPE32INT)

#define MAX_HANDLERS 50

static handler_t handlers[IDT_NUM_ENTRIES][MAX_HANDLERS];
static uint32_t handlers_heads[IDT_NUM_ENTRIES] = { 0 };

static void dump_registers(registers_t *regs);
extern char *exception_messages[];

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr127();
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

void arch_init()
{
        gdt_ptr_t *gdtp = gdt_setup_pointer();
        idt_ptr_t *idtp = idt_setup_pointer();

        uint16_t kcode = gdt_set_gate(gdtp, GDT_INDEX_KCODE, 0x00000000, 0xffffffff, ACCESS_KCODE, GDT_FLAGS);
        uint16_t ucode = gdt_set_gate(gdtp, GDT_INDEX_UCODE, 0x00000000, 0xffffffff, ACCESS_UCODE, GDT_FLAGS);
        uint16_t kdata = gdt_set_gate(gdtp, GDT_INDEX_KDATA, 0x00000000, 0xffffffff, ACCESS_KDATA, GDT_FLAGS);
        uint16_t udata = gdt_set_gate(gdtp, GDT_INDEX_UDATA, 0x00000000, 0xffffffff, ACCESS_UDATA, GDT_FLAGS);

        gdt_flush(gdtp);
        
        idt_set_gate(idtp, 0,  isr0,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 1,  isr1,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 2,  isr2,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 3,  isr3,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 4,  isr4,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 5,  isr5,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 6,  isr6,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 7,  isr7,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 8,  isr8,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 9,  isr9,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 10, isr10, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 11, isr11, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 12, isr12, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 13, isr13, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 14, isr14, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 16, isr16, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 17, isr17, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 18, isr18, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 19, isr19, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 20, isr20, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 21, isr21, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 22, isr22, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 23, isr23, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 24, isr24, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 25, isr25, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 26, isr26, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 27, isr27, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 28, isr28, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 29, isr29, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 30, isr30, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, 31, isr31, kcode, IDT_FLAGS_RING0);

        pic_init();
        pic_disable();
        
        idt_set_gate(idtp, IRQ(0),  irq0,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(1),  irq1,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(2),  irq2,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(3),  irq3,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(4),  irq4,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(5),  irq5,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(6),  irq6,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(7),  irq7,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(8),  irq8,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(9),  irq9,  kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(10), irq10, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(11), irq11, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(12), irq12, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(13), irq13, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(14), irq14, kcode, IDT_FLAGS_RING0);
        idt_set_gate(idtp, IRQ(15), irq15, kcode, IDT_FLAGS_RING0);

        idt_set_gate(idtp, SYSCALL_VECTOR, isr127, kcode, IDT_FLAGS_RING3);

        idt_flush(idtp);

        pit_init(TIMER_FREQ);

        (void)ucode;
        (void)kdata;
        (void)udata;
}

void arch_finish()
{
        pic_disable();
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

void enable_irq(uint8_t irq)
{
        pic_enable_irq(irq);
}

void disable_irq(uint8_t irq)
{
        pic_disable_irq(irq);
}

void disable_irqs()
{
        pic_disable();
}

void restore_irqs()
{
        //pic_restore();
}

void sleep(uint32_t ms)
{
        uint32_t current = pit_get_ticks();

        while (current + ms > pit_get_ticks()) ;
}

uint32_t get_ticks_count()
{
        return pit_get_ticks();
}

void isr_handler(void *r)
{
        registers_t *regs = (registers_t *)r;
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
                halt();
        }
}

void irq_handler(void *r)
{
        registers_t *regs = (registers_t *)r;
        handler_t *h = 0;

        if (pic_acknowledge(regs->int_no)) {
                kprintf(NOTICE, "Spurious IRQ");
                return; /* ignore spurious IRQs */
        }

        h = get_interrupt_handler(IRQ(regs->int_no));
        if (!h) {
                kprintf(WARNING, "No handler for IRQ #%u", regs->int_no);
                return;
        }
        while (h) {
                h->handler(regs);
                h = h->next;
        }
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
                "es: %#04x fs: %#04x gs: %#04x\n",
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
