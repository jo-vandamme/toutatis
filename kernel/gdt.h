#ifndef __KERNEL_GDT_H__
#define __KERNEL_GDT_H__

#include <types.h>

#define GDT_NUM_ENTRIES 6

/* Access byte flags */
#define GDT_ACCESS_ACCESSED     (0x01 << 0) /* accessed bit (set to 0, the cpu sets this to 1 when the segment is accessed) */
#define GDT_ACCESS_RW           (0x01 << 1) /* readable bit for code selector, writable bit for data selector */
#define GDT_ACCESS_DC           (0x01 << 2) /* direction/conforming bit */
#define GDT_ACCESS_EXECUTE      (0x01 << 3) /* executable bit (1 for code selector, 0 for data selector) */
#define GDT_ACCESS_ALWAYS1      (0x01 << 4) /* descriptor type (0 for system, 1 for code/data) */
#define GDT_ACCESS_RING0        (0x00 << 5)
#define GDT_ACCESS_RING1        (0x01 << 5)
#define GDT_ACCESS_RING2        (0x02 << 5)
#define GDT_ACCESS_RING3        (0x03 << 5)
#define GDT_ACCESS_PRESENT      (0x01 << 7) /* present bit, must be 1 for for all valid selectors */

/* Flag byte flags */
#define GDT_FLAG_32BIT          (0x01 << 6) /* operation size (0 for 16 bit, 1 for 32) */
#define GDT_FLAG_GRANULARITY    (0x01 << 7) /* granularity (0 for 1B-1MB, 1 for 4KB-4GB) */

typedef enum
{
    GDT_INDEX_NULL  = 0x00,
    GDT_INDEX_KCODE = 0x01,
    GDT_INDEX_KDATA = 0x02,
    GDT_INDEX_UCODE = 0x03,
    GDT_INDEX_UDATA = 0x04,
    GDT_INDEX_TSS   = 0x05
} gdt_index_t;

typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  flags;
    uint8_t  base_high;
}  __attribute__((packed)) gdt_entry_t;

typedef struct
{
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trap;
    uint32_t iomap_base;
} __attribute__((packed)) tss_entry_t;

typedef struct
{
    uint16_t limit;    /* size of the table minus one (last valid address in the table) */
    gdt_entry_t *base; /* address of first GDT entry */
}  __attribute__((packed)) gdt_ptr_t;

//void gdt_set_gate(gdt_index_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
void gdt_init();

#endif
