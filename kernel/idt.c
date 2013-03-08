#include <string.h>
#include "idt.h"

static idt_entry_t entries[IDT_NUM_ENTRIES];
static idt_ptr_t   pointer;

void idt_set_entry(idt_ptr_t *p, u8_t index, void (*callback)(), u16_t selector, u8_t flags)
{
        u32_t base = (u32_t)callback;

        p->base[index].base_low  = base & 0xffff;
        p->base[index].base_high = base >> 16;
        p->base[index].selector  = selector;
        p->base[index].flags     = flags;
}

idt_ptr_t *idt_setup_pointer()
{
        memset(&entries, 0, sizeof(idt_entry_t) * IDT_NUM_ENTRIES);

        pointer.base = entries;
        pointer.limit = sizeof(idt_entry_t) * IDT_NUM_ENTRIES - 1;

        return &pointer;
}
