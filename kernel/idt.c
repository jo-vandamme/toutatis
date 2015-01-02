#include <system.h>
#include <string.h>
#include <idt.h>

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
extern void isr128();

static idt_entry_t idt_entries[IDT_NUM_ENTRIES];
static idt_ptr_t   idt_ptr;

void idt_set_gate(uint8_t index, void (*callback)(), uint16_t selector, uint8_t flags)
{
    uint32_t base = (uint32_t)callback;

    idt_entries[index].base_low  = base & 0xffff;
    idt_entries[index].base_high = (base >> 16) & 0xffff;
    idt_entries[index].selector  = selector;
    idt_entries[index].zero      = 0;
    idt_entries[index].flags     = flags;
}

void idt_init()
{
    memset(&idt_entries, 0, sizeof(idt_entry_t) * IDT_NUM_ENTRIES);

    idt_ptr.base = idt_entries;
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_NUM_ENTRIES - 1;

    idt_set_gate(0,  isr0,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(1,  isr1,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(2,  isr2,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(3,  isr3,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(4,  isr4,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(5,  isr5,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(6,  isr6,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(7,  isr7,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(8,  isr8,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(9,  isr9,  KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(10, isr10, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(11, isr11, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(12, isr12, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(13, isr13, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(14, isr14, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(16, isr16, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(17, isr17, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(18, isr18, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(19, isr19, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(20, isr20, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(21, isr21, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(22, isr22, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(23, isr23, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(24, isr24, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(25, isr25, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(26, isr26, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(27, isr27, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(28, isr28, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(29, isr29, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(30, isr30, KCODE_SEL, IDT_FLAGS_RING0);
    idt_set_gate(31, isr31, KCODE_SEL, IDT_FLAGS_RING0);

    idt_set_gate(SYSCALL_VECTOR, isr128, KCODE_SEL, IDT_FLAGS_RING3);

    idt_flush(&idt_ptr);
}
