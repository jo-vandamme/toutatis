#include <serial.h>
#include <arch.h>
#include <types.h>
#include <vsprintf.h>

#define SERIAL_DATA(base)               (base)
#define SERIAL_DLL(base)                (base + 0) /* divisor latch low byte */
#define SERIAL_DLH(base)                (base + 1) /* divisor latch high byte */
#define SERIAL_INT_ENABLE(base)         (base + 1) /* interrupt enable register */
#define SERIAL_INT_IDENT(base)          (base + 2) /* interrupt identification register */
#define SERIAL_FIFO_CTRL(base)          (base + 2) /* FIFO control register */
#define SERIAL_LINE_CTRL(base)          (base + 3) /* line control register */
#define SERIAL_MODEM_CTRL(base)         (base + 4) /* modem control register */
#define SERIAL_LINE_STATUS(base)        (base + 5) /* line status register */
#define SERIAL_MODEM_STATUS(base)       (base + 6) /* modem status register */
#define SERIAL_SCRATCH(base)            (base + 7) /* scratch register */

static u16_t com;
static device_t com_device;

static int is_transmit_empty(void);
static void write_char(char c);

static const char *term_fg[] =
{
        "\033[0;30;%sm", /* black */
        "\033[0;34;%sm", /* blue */
        "\033[0;32;%sm", /* green */
        "\033[0;36;%sm", /* cyan */
        "\033[0;31;%sm", /* red */
        "\033[0;35;%sm", /* magenta */
        "\033[0;33;%sm", /* brown */
        "\033[0;37;%sm", /* light gray */
        "\033[0;90;%sm", /* dark gray */
        "\033[0;94;%sm", /* light blue */
        "\033[0;92;%sm", /* light green */
        "\033[0;96;%sm", /* light cyan */
        "\033[0;91;%sm", /* light red */
        "\033[0;95;%sm", /* light magenta */
        "\033[0;33;%sm", /* yellow */
        "\033[0;37;%sm", /* white */
};

static const char *term_bg[] =
{
        "40",  /* black */
        "44",  /* blue */
        "42",  /* green */
        "46",  /* cyan */
        "41",  /* red */
        "45",  /* magenta */
        "43",  /* brown */
        "47",  /* light gray */
        "100", /* dark gray */
        "104", /* light blue */
        "102", /* light green */
        "106", /* light cyan */
        "101", /* light red */
        "105", /* light magenta */
        "103", /* yellow */
        "47",  /* white */
};

#if 0
static int serial_received(void);
static char read_char(void);
#endif

device_t *serial_init(void)
{
        const char *init_message = "\n\033[4;35;40mSerial output\033[0;37;40m\n";

        u16_t divisor = 3; /* 115200 / 3 = 38400 Hz */

        /* get the base address of COM1 port in the BDA */
        com = *((u16_t *)0x0400);

        outb(SERIAL_INT_ENABLE(com), 0x00);     /* disable interrupts */
        outb(SERIAL_LINE_CTRL(com), 0x80);      /* enable DLAB */
        outb(SERIAL_DLL(com), divisor);         /* send divisor low byte */
        outb(SERIAL_DLH(com), divisor >> 8);    /* send divisor high byte */

        /* Bit:     | 7    | 6     | 5 4 3  | 2    | 1 0     |
         * Content: | DLAB | break | parity | stop | length  |
         * Value:   | 0    | 0     | 0 0 0  | 0    | 1 1     | = 0x03
         * Means: length of 8 bits, no parity bit, one stop bit and break control disabled.
         */
        outb(SERIAL_LINE_CTRL(com), 0x03);

        /* Bit:     | 7 6         | 5        | 4   | 3        | 2              | 1               | 0            |
         * Content: | trig. level | 64B FIFO | res | DMA mode | clear tr. FIFO | clear rec. FIFO | enable FIFOs |
         * Value:   | 1 1         | 0        | 0   | 0        | 1              | 1               | 1            | = 0xc7
         * Means: 14 bytes stored trigger interrupt, 16 bytes buffer, clear both receiver and
         *        transmission FIFO queues, enable FIFOs.
         * http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming#FIFO_Control_Register
         */
        outb(SERIAL_FIFO_CTRL(com), 0xc7);

        /* Bit:     | 7 6 | 5        | 4        | 3           | 2           | 1   | 0   |
         * Content: | res | autoflow | loopback | aux. out. 2 | aux. out. 1 | rts | dtr |
         * Value:   | 0 0 | 0        | 0        | 1           | 0           | 1   | 1   | = 0x0b
         * Means: request to send (RTS) and data terminal ready (DTR) meaning we are ready to
         *        send data, enable interrupts (auxiliary output 2 = 1).
         */
        outb(SERIAL_MODEM_CTRL(com), 0x0b);

        //outb(SERIAL_INT_ENABLE(com), 0x01); /* enable interrupts on receive */

        com_device.read = 0;
        com_device.write = write;

        while (*init_message) {
                write_char(*init_message++);
        }

        return &com_device;
}

void serial_terminate(void)
{
        const char *text_reset = "\033[0m";

        while (*text_reset) {
                write_char(*text_reset++);
        }
}

size_t write(u8_t *data, size_t len)
{
        size_t i;
        const char *fmt;
        char buf[32] = { 0 };
        unsigned k = 0;

        for (i = 0; *data && i < len; ++data, ++i) {
                if (*data == '\033') {
                        ++data;
                        ++i;
                        fmt = term_fg[*data % 16];
                        sprintf(buf, fmt, term_bg[*data / 16]);
                        for (k = 0; buf[k]; ++k) {
                                write_char(buf[k]);
                        }
                } else {
                        write_char(*data);
                }
        }
        return i;
}

static int is_transmit_empty(void)
{
        return inb(SERIAL_LINE_STATUS(com)) & 0x20;
}

static void write_char(char c)
{
        while (is_transmit_empty() == 0) ;
        outb(SERIAL_DATA(com), c);
}

#if 0
static int serial_received(void)
{
        return inb(SERIAL_LINE_STATUS(com)) & 0x01;
}

static char read_char(void)
{
        if (serial_received() == 0)
                return 0;
        return inb(SERIAL_DATA(com));
}
#endif

