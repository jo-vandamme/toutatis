#include <system.h>
#include <string.h>
#include <gdt.h>

#define ACCESS_KCODE (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW | GDT_ACCESS_EXECUTE)
#define ACCESS_UCODE (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW | GDT_ACCESS_EXECUTE)
#define ACCESS_KDATA (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW)
#define ACCESS_UDATA (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_ALWAYS1 | GDT_ACCESS_RW)
#define GDT_FLAGS    (GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT)

static gdt_entry_t gdt_entries[GDT_NUM_ENTRIES];
static gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

static void gdt_set_gate(gdt_index_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    gdt_entries[index].base_low    = base & 0xffff;
    gdt_entries[index].base_middle = (base >> 16) & 0xff;
    gdt_entries[index].base_high   = (base >> 24) & 0xff;
        
    gdt_entries[index].limit_low   = limit & 0xffff;
    gdt_entries[index].flags       = (flags & 0xf0) | ((limit >> 16) & 0x0f);

    gdt_entries[index].access      = access;
}

static void write_tss(gdt_index_t index, uint16_t ss0, uint32_t esp0)
{
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = base + sizeof(tss_entry_t);

    gdt_set_gate(index, base, limit, 0xe9, 0x00);

    memset(&tss_entry, 0, sizeof(tss_entry_t));

    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;
    tss_entry.cs = 0x0b;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
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
    
    write_tss(GDT_INDEX_TSS, 0x10, 0x00);

    gdt_flush(&gdt_ptr);
    tss_flush();
}

