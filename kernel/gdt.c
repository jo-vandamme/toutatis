#include <string.h>
#include "gdt.h"

static gdt_entry_t entries[GDT_NUM_ENTRIES];
static gdt_ptr_t   pointer;

u16_t gdt_set_entry(gdt_ptr_t *p, gdt_index_t index, u32_t base, u32_t limit, u8_t access, u8_t flags)
{
        p->base[index].base_low    = base & 0xffff;
        p->base[index].base_middle = (base >> 16) & 0xff;
        p->base[index].base_high   = (base >> 24) & 0xff;
        
        p->base[index].limit_low   = limit & 0xffff;
        p->base[index].flags       = (flags & 0xf0) | ((limit >> 16) & 0x0f);

        p->base[index].access      = access;

        return (sizeof(gdt_entry_t) * index) | ((access >> 5) & 0x03);
}

gdt_ptr_t *gdt_setup_pointer()
{
        memset(&entries, 0, sizeof(gdt_entry_t) * GDT_NUM_ENTRIES);

        pointer.base = entries;
        pointer.limit = sizeof(gdt_entry_t) * GDT_NUM_ENTRIES - 1;

        return &pointer;
}

