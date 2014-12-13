#include <string.h>
#include "idt.h"

static idt_entry_t entries[IDT_NUM_ENTRIES];
static idt_ptr_t   pointer;

void idt_set_gate(idt_ptr_t *p, uint8_t index, void (*callback)(), uint16_t selector, uint8_t flags)
{
        uint32_t base = (uint32_t)callback;

        p->base[index].base_low  = base & 0xffff;
        p->base[index].base_high = (base >> 16) & 0xffff;
        p->base[index].selector  = selector;
        p->base[index].zero      = 0;
        p->base[index].flags     = flags;
}

idt_ptr_t *idt_setup_pointer()
{
        memset(&entries, 0, sizeof(idt_entry_t) * IDT_NUM_ENTRIES);

        pointer.base = entries;
        pointer.limit = sizeof(idt_entry_t) * IDT_NUM_ENTRIES - 1;

        return &pointer;
}
