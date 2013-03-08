#include "pit.h"
#include "pic.h"
#include "arch.h"
#include <vga.h>

#define IRQ_TIMER                       0

#define PIT_MAX_FREQ                    1193182
#define PIT_DATA0                       0x40
#define PIT_DATA1                       0x41
#define PIT_DATA2                       0x42
#define PIT_CTRL                        0x43

#define PIT_OCW_BINCOUNT_BINARY         0x00
#define PIT_OCW_MODE_TERMINAL_COUNT     0x00
#define PIT_OCW_MODE_ONE_SHOT           0x02
#define PIT_OCW_MODE_RATE               0x04
#define PIT_OCW_MODE_SQUARE_WAVE        0x06
#define PIT_OCW_RL_LSB_THEN_MSB         0x30
#define PIT_OCW_COUNTER0                0x00
#define PIT_OCW_COUNTER1                0x40
#define PIT_OCW_COUNTER2                0x80

static volatile u32_t ticks = 0;

static void pit_handler(registers_t *r)
{
        (void)r;
        ++ticks;
}

void pit_init(u32_t freq)
{
        if (freq == 0)
                return;

        u16_t divisor = (u16_t)(PIT_MAX_FREQ / freq);

        outb(PIT_CTRL, PIT_OCW_COUNTER0 |
                       PIT_OCW_BINCOUNT_BINARY |
                       PIT_OCW_RL_LSB_THEN_MSB |
                       PIT_OCW_MODE_SQUARE_WAVE);

        outb(PIT_DATA0, divisor & 0xff); /* send lower byte */
        outb(PIT_DATA0, divisor >> 8);   /* send upper byte */

        ticks = 0;

        attach_interrupt_handler(IRQ(IRQ_TIMER), pit_handler);
        pic_enable_irq(IRQ_TIMER);
}

u32_t pit_get_ticks()
{
        return ticks;
}

void pit_set_ticks(u32_t val)
{
        ticks = val;
}

