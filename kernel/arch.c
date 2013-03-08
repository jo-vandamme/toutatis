#include "arch.h"
#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include <vga.h>

#define ACCESS_KCODE (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW | GDT_ACCESS_EXECUTE)
#define ACCESS_UCODE (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW | GDT_ACCESS_EXECUTE)
#define ACCESS_KDATA (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW)
#define ACCESS_UDATA (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW)
#define GDT_FLAGS    (GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT)

#define MAX_HANDLERS 50

static handler_t handlers[IDT_NUM_ENTRIES][MAX_HANDLERS];
static u32_t handlers_heads[IDT_NUM_ENTRIES] = { 0 };

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
/* 15 is unassigned */
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
/* 20-31 are reserved */
extern void isr128();
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

static void setup_tables(gdt_ptr_t *gdtp, idt_ptr_t *idtp)
{
        u16_t kcode = gdt_set_entry(gdtp, GDT_INDEX_KCODE, 0x00000000, 0xffffffff, ACCESS_KCODE, GDT_FLAGS);
        u16_t ucode = gdt_set_entry(gdtp, GDT_INDEX_UCODE, 0x00000000, 0xffffffff, ACCESS_UCODE, GDT_FLAGS);
        u16_t kdata = gdt_set_entry(gdtp, GDT_INDEX_KDATA, 0x00000000, 0xffffffff, ACCESS_KDATA, GDT_FLAGS);
        u16_t udata = gdt_set_entry(gdtp, GDT_INDEX_UDATA, 0x00000000, 0xffffffff, ACCESS_UDATA, GDT_FLAGS);

        idt_set_entry(idtp, 0,  isr0,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 1,  isr1,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 2,  isr2,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 3,  isr3,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 4,  isr4,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 5,  isr5,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 6,  isr6,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 7,  isr7,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 8,  isr8,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 9,  isr9,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 10, isr10, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 11, isr11, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 12, isr12, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 13, isr13, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 14, isr14, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 16, isr16, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 17, isr17, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 18, isr18, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, 19, isr19, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);

        idt_set_entry(idtp, IRQ(0),  irq0,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(1),  irq1,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(2),  irq2,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(3),  irq3,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(4),  irq4,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(5),  irq5,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(6),  irq6,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(7),  irq7,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(8),  irq8,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(9),  irq9,  kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(10), irq10, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(11), irq11, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(12), irq12, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(13), irq13, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(14), irq14, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
        idt_set_entry(idtp, IRQ(15), irq15, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);

        idt_set_entry(idtp, SYSCALL_VECTOR, isr128, kcode, IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_FLAG_TYPE32INT);

        cpu_set_gdt(gdtp);
        cpu_set_idt(idtp);

        (void)ucode;
        (void)kdata;
        (void)udata;
}

void arch_init()
{
        gdt_ptr_t *gdtp = gdt_setup_pointer();
        idt_ptr_t *idtp = idt_setup_pointer();

        setup_tables(gdtp, idtp);

        pic_init();
        pic_disable();

        pit_init(TIMER_FREQ);
}

void arch_finish()
{
        pic_disable();
}

void attach_interrupt_handler(u8_t num, isr_t handler)
{
        handler_t *h = 0, *n = 0;
        u8_t i = 0;
        u32_t head;

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

void detach_interrupt_handler(u8_t num, isr_t handler)
{
        /* get the head */
        u32_t head = handlers_heads[num];
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
                                        handlers_heads[num] = (u32_t)(h->next - &handlers[num][0]);
                                        /* if h->next is 0, the list is empty and head will be 0 */
                                }
                        }
                        h->handler = 0;
                        sti();
                        break;
                }
        } while ((h = h->prev) != 0);
}

handler_t *get_interrupt_handler(u8_t num)
{
        u32_t head = handlers_heads[num];
        handler_t *h = &handlers[num][head];

        if (h->handler) {
                return h;
        } else {
                return 0;
        }
}

void enable_irq(u8_t irq)
{
        pic_enable_irq(irq);
}

void disable_irq(u8_t irq)
{
        pic_disable_irq(irq);
}

void disable_irqs()
{
        pic_disable();
}

void restore_irqs()
{
        pic_restore();
}

void sleep(u32_t ms)
{
        u32_t current = pit_get_ticks();

        while (current + ms > pit_get_ticks()) ;
}

u32_t get_ticks_count()
{
        return pit_get_ticks();
}

void isr_handler(void *r)
{
        registers_t *regs = (registers_t *)r;
        u8_t stop = 0;

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
                vga_set_attribute(0x0c00);
                vga_print_str("Unhandled exception #");
                vga_print_dec(regs->int_no);
                if (regs->int_no < IRQ(0)) {
                        vga_print_str(" (");
                        vga_print_str(exception_messages[regs->int_no]);
                        vga_print_str(")");
                }
                vga_print_str("\n");
                dump_registers(regs);
                halt();
        }
}

void irq_handler(void *r)
{
        registers_t *regs = (registers_t *)r;
        handler_t *h = 0;

        if (pic_acknowledge(regs->int_no)) {
                vga_print_str("spurious IRQ");
                return; /* ignore spurious IRQs */
        }

        h = get_interrupt_handler(IRQ(regs->int_no));
        if (!h) {
                vga_print_str("no handler for IRQ #");
                vga_print_dec(regs->int_no);
                return;
        }
        while (h) {
                h->handler(regs);
                h = h->next;
        }
}

static void dump_registers(registers_t *regs)
{
        vga_print_str("EAX: ");    vga_print_hex(regs->eax);
        vga_print_str("\tEBX: ");  vga_print_hex(regs->ebx);
        vga_print_str("\tECX: ");  vga_print_hex(regs->ecx); newline;
        vga_print_str("EDX: ");    vga_print_hex(regs->edx);
        vga_print_str("\tESI: ");  vga_print_hex(regs->esi);
        vga_print_str("\tEDI: ");  vga_print_hex(regs->edi); newline;
        vga_print_str("EBP: ");    vga_print_hex(regs->ebp);
        vga_print_str("\tESP: ");  vga_print_hex(regs->esp);
        vga_print_str("\tEIP: ");  vga_print_hex(regs->eip);
        vga_print_str("EFL: ");    vga_print_hex(regs->eflags); newline;
        vga_print_str("SS: ");     vga_print_hex(regs->ss);
        vga_print_str("\tCS: ");   vga_print_hex(regs->cs);
        vga_print_str("\tDS: ");   vga_print_hex(regs->ds);
        vga_print_str("\tES: ");   vga_print_hex(regs->es);
        vga_print_str("\tFS: ");   vga_print_hex(regs->fs);
        vga_print_str("\tGS: ");   vga_print_hex(regs->gs); newline;
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
