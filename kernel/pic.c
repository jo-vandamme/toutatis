#include <system.h>
#include <idt.h>
#include <pic.h>

#define PIC1_CTRL       0x20        /* master pic */
#define PIC2_CTRL       0xa0        /* slave pic */
#define PIC1_DATA       0x21
#define PIC2_DATA       0xa1
#define PIC_EOI         0x20        /* end-of-interrupt command code */
#define PIC_READ_ISR    0x0b        /* command to read the In-Service Register */

#define ICW1_ICW4       0x01        /* ICW4 (not) needed */
#define ICW1_SINGLE     0x02        /* single (cascade) mode */
#define ICW1_INTERVAL4  0x04        /* call address interval 4 (8) */
#define ICW1_LEVEL      0x08        /* level triggered (edge) mode */
#define ICW1_INIT       0x10        /* initialization */

#define ICW4_8086       0x01        /* 8086/88 mode (MCS-80/85) mode */
#define ICW4_AUTO       0x02        /* auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08        /* buffered mode/slave */
#define ICW4_BUF_MASTER 0x0c        /* buffered mode/master */
#define ICW4_SFNM       0x10        /* special fully nested (not) */

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

static uint32_t bad_irqs = 0;

void pic_init()
{
    pic_remap();
    pic_disable();
    irq_gates();
    INT_ON;
}

void irq_gates()
{
    idt_set_gate(IRQ(0),  irq0,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(1),  irq1,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(2),  irq2,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(3),  irq3,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(4),  irq4,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(5),  irq5,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(6),  irq6,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(7),  irq7,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(8),  irq8,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(9),  irq9,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(10), irq10, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(11), irq11, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(12), irq12, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(13), irq13, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(14), irq14, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(IRQ(15), irq15, KCODE_SEL, IDT_FLAGS_RING0);
}

void pic_remap()
{
    /* save mask */
    uint32_t mask = inb(PIC1_DATA) | inb(PIC2_DATA) << 8;

    /* ICW1: start the initialization sequence (in cascade mode) */
    outb(PIC1_CTRL, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CTRL, ICW1_INIT | ICW1_ICW4);

    /* ICW2: Master PIC vector offset */
    outb(PIC1_DATA, 0x20);
    /* ICW2: Slave PIC vector offset */
    outb(PIC2_DATA, 0x28);

    /* ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100) */
    outb(PIC1_DATA, 4);
    /* ICW3: tell Slave PIC its cascade identity (0000 0010) */
    outb(PIC2_DATA, 2);

    /* ICW4: controls how evrything is to operate */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    /* restore saved mask */
    outb(PIC1_DATA, mask);
    outb(PIC2_DATA, mask >> 8);
}

void pic_disable()
{
    /* deactivate all irqs, except irq 2 since this
     * is the connection to the slave pic */
    outb(PIC1_DATA, 0xfb);
    outb(PIC2_DATA, 0xff);
}

void pic_disable_irq(pic_index_t irq)
{
    uint16_t port;
    uint8_t mask;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    mask = inb(port) | (1 << irq);
    outb(port, mask);
}

void pic_enable_irq(pic_index_t irq)
{
    uint16_t port;
    uint8_t mask;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    mask = inb(port) & ~(1 << irq);
    outb(port, mask);
}

uint32_t pic_get_bad_irqs()
{
    return bad_irqs;
}

static uint16_t pic_get_irq_reg(uint16_t command)
{
    outb(PIC1_CTRL, command);
    outb(PIC2_CTRL, command);
    return (inb(PIC2_CTRL) << 8) | inb(PIC1_CTRL);
}

static uint16_t pic_get_isr()
{
    /* Tells us which interrupt are being serviced */
    return pic_get_irq_reg(PIC_READ_ISR);
}

static uint8_t pic_is_spurious_7()
{
    return (pic_get_isr() & 0x0080) == 0;
}

static uint8_t pic_is_spurious_15()
{
    return (pic_get_isr() & 0x8000) == 0;
}

uint8_t pic_acknowledge(pic_index_t irq)
{
    /* handle spurious IRQs */
    if (irq == 7  && pic_is_spurious_7()) {
        ++bad_irqs;
        return 1;
    }
    if (irq == 15 && pic_is_spurious_15()) {
        /* send an EOI to the master PIC only since it can't
         * know if it was a spurious irq from the slave */
        outb(PIC1_CTRL, PIC_EOI);
        ++bad_irqs;
        return 1;
    }

    /* Issue the end-of-interrupt command to the PIC.
     * If the IRQ came from the Master PIC, it is sufficient
     * to issue this command only to the Master PIC; however if the IRQ came from
     * the Slave PIC, it is necessary to issue the command to both PIC chips. */
    if (irq >= 8) {
        outb(PIC2_CTRL, PIC_EOI);
    }
    outb(PIC1_CTRL, PIC_EOI);
    return 0;
}
