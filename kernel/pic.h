#ifndef __KERNEL_PIC_H__
#define __KERNEL_PIC_H__

#include <types.h>

typedef enum
{
    PIC_INDEX_PIT       = 0x00,
    PIC_INDEX_KBD       = 0x01,
    PIC_INDEX_SLAVE     = 0x02,
    PIC_INDEX_COM2      = 0x03,
    PIC_INDEX_COM1      = 0x04,
    PIC_INDEX_LPT2      = 0x05,
    PIC_INDEX_FLOPPY    = 0x06,
    PIC_INDEX_LPT1      = 0x07,
    PIC_INDEX_RTC       = 0x08,
    PIC_INDEX_CGA       = 0x09,
    PIC_INDEX_RES1      = 0x0a,
    PIC_INDEX_RES2      = 0x0b,
    PIC_INDEX_MOUSE     = 0x0c,
    PIC_INDEX_FPU       = 0x0d,
    PIC_INDEX_HD        = 0x0e,
    PIC_INDEX_RES3      = 0x0f
} pic_index_t;

void pic_init();
void pic_remap();
void irq_gates();
void pic_enable_irq(pic_index_t irq);
void pic_disable_irq(pic_index_t irq);
void pic_disable();
void pic_restore();

uint8_t pic_acknowledge(pic_index_t irq);
uint32_t pic_get_bad_irqs();

#endif
