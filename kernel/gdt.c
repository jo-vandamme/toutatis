#include <system.h>
#include <gdt.h>

#define ACCESS_KCODE (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW | GDT_ACCESS_EXECUTE)
#define ACCESS_UCODE (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW | GDT_ACCESS_EXECUTE)
#define ACCESS_KDATA (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW)
#define ACCESS_UDATA (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW)
#define GDT_FLAGS    (GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT)

static gdt_entry_t gdt_entries[GDT_NUM_ENTRIES];
static gdt_ptr_t   gdt_ptr;

void gdt_set_gate(gdt_index_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    gdt_entries[index].base_low    = base & 0xffff;
    gdt_entries[index].base_middle = (base >> 16) & 0xff;
    gdt_entries[index].base_high   = (base >> 24) & 0xff;
        
    gdt_entries[index].limit_low   = limit & 0xffff;
    gdt_entries[index].flags       = (flags & 0xf0) | ((limit >> 16) & 0x0f);

    gdt_entries[index].access      = access;
}

void gdt_init()
{
    gdt_ptr.base = gdt_entries;
    gdt_ptr.limit = sizeof(gdt_entry_t) * GDT_NUM_ENTRIES - 1;

    gdt_set_gate(GDT_INDEX_NULL, 0, 0, 0, 0);
    gdt_set_gate(GDT_INDEX_KCODE, 0x00000000, 0xffffffff, ACCESS_KCODE, GDT_FLAGS);
    gdt_set_gate(GDT_INDEX_UCODE, 0x00000000, 0xffffffff, ACCESS_UCODE, GDT_FLAGS);
    gdt_set_gate(GDT_INDEX_KDATA, 0x00000000, 0xffffffff, ACCESS_KDATA, GDT_FLAGS);
    gdt_set_gate(GDT_INDEX_UDATA, 0x00000000, 0xffffffff, ACCESS_UDATA, GDT_FLAGS);

    gdt_flush(&gdt_ptr);
}
