#ifndef __KERNEL_GDT_H__
#define __KERNEL_GDT_H__

#include <types.h>

#define GDT_NUM_ENTRIES         5
#define GDT_ACCESS_ACCESSED     (0x01 << 0) /* accessed bit (set to 0, the cpu sets this to 1 when the segment is accessed) */
#define GDT_ACCESS_RW           (0x01 << 1) /* readable bit for code selector, writable bit for data selector */
#define GDT_ACCESS_DC           (0x01 << 2) /* direction/conforming bit */
#define GDT_ACCESS_EXECUTE      (0x01 << 3) /* executable bit (1 for code selector, 0 for data selector) */
#define GDT_ACCESS_ALWAYS1      (0x01 << 4) /* descriptor type (0 for system, 1 for code/data) */
#define GDT_ACCESS_RING0        (0x00 << 5)
#define GDT_ACCESS_RING1        (0x01 << 5)
#define GDT_ACCESS_RING2        (0x02 << 5)
#define GDT_ACCESS_RING3        (0x03 << 5)
#define GDT_ACCESS_PRESENT      (0x01 << 7)

#define GDT_FLAG_32BIT          (0x01 << 6) /* operation size (0 for 16 bit, 1 for 32) */
#define GDT_FLAG_GRANULARITY    (0x01 << 7) /* granularity (0 for 1B-1MB, 1 for 4KB-4GB) */

typedef enum
{
        GDT_INDEX_KCODE         = 0x01,
        GDT_INDEX_KDATA         = 0x02,
        GDT_INDEX_UCODE         = 0x03,
        GDT_INDEX_UDATA         = 0x04
} gdt_index_t;

typedef struct
{
        u16_t limit_low;
        u16_t base_low;
        u8_t  base_middle;
        u8_t  access;
        u8_t  flags;
        u8_t  base_high;
}  __attribute__((packed)) gdt_entry_t;

typedef struct
{
        u16_t limit;
        gdt_entry_t *base;
}  __attribute__((packed)) gdt_ptr_t;

u16_t gdt_set_entry(gdt_ptr_t *p, gdt_index_t index, u32_t base, u32_t limit, u8_t access, u8_t flags);
gdt_ptr_t *gdt_setup_pointer();

#endif
