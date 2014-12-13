#include "keyboard.h"
#include "arch.h"
#include "pic.h"
#include "vga.h"

#define IRQ_KBD     1
#define KBD_DATA    0x60
#define KBD_CTRL    0x64

#define LSHIFT      0x2a
#define RSHIFT      0x36
#define CAPSLOCK    0x1d

static uint32_t shift_pressed = 0;

#define KBD_BUF_SIZE    32
static uint8_t kbd_buffer[KBD_BUF_SIZE];
static uint8_t read_idx = 0;
static volatile uint8_t write_idx = 0;

const uint8_t keymap_us[2][128] =
{
    {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', /*  0-14 */
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',  /* 15-28 */
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',          /* 29-41 */
        0, '\\', 'z','x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,             /* 42-54 */
        0,   /* 55 - Ctrl */
        0,   /* 56 - Alt */
        ' ', /* 57 - Space */
        0,   /* 58 - Caps lock */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 59-68 - F1-F10 */
        0, /* 69 - Num lock */
        0, /* 70 - Scroll lock */
        0, /* 71 - Home */
        0, /* 72 - Up arrow */
        0, /* 73 - Page up */
        0,
        0, /* 75 - Left arrow */
        0,
        0, /* 77 - Right arrow */
        0,
        0, /* 79 - End */
        0, /* 80 - Down arrow */
        0, /* 81 - Page down */
        0, /* 82 - Insert */
        0, /* 83 - Delete */
        0, 0, 0,
        0, /* 87 - F11 */
        0, /* 88 - F12 */
        0, 0, 0, 0,
        0, /* 93 - Right click key */
        0, /* All other keys are undefined */
    },
    {
        0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', /*  0-14 */
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',  /* 15-28 */
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',           /* 29-41 */
        0, '\\', 'Z','X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,             /* 42-54 */
        0,   /* 55 - Ctrl */
        0,   /* 56 - Alt */
        ' ', /* 57 - Space */
        0,   /* 58 - Caps lock */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 59-68 - F1-F10 */
        0, /* 69 - Num lock */
        0, /* 70 - Scroll lock */
        0, /* 71 - Home */
        0, /* 72 - Up arrow */
        0, /* 73 - Page up */
        0,
        0, /* 75 - Left arrow */
        0,
        0, /* 77 - Right arrow */
        0,
        0, /* 79 - End */
        0, /* 80 - Down arrow */
        0, /* 81 - Page down */
        0, /* 82 - Insert */
        0, /* 83 - Delete */
        0, 0, 0,
        0, /* 87 - F11 */
        0, /* 88 - F12 */
        0, 0, 0, 0,
        0, /* 93 - Right click key */
        0, /* All other keys are undefined */
    }
};

static void keyboard_handler(registers_t *r)
{
    (void)r;
    uint8_t scancode = inb(KBD_DATA);
        
    /* if the top bit of the byte is set, a key has just been released */
    if (scancode & 0x80)
    {
        scancode &= 0x7f;
        if (scancode == LSHIFT || scancode == RSHIFT)
        {
            shift_pressed = 0;
        }
    }
    else /* a key has just been pressed */
    {
        if (scancode == LSHIFT || scancode == RSHIFT)
        {
            shift_pressed = 1;
        }
        else
        {
            vga_print_char(keymap_us[shift_pressed][scancode]);

            if (((write_idx + 1) % KBD_BUF_SIZE) != read_idx)
            {
                kbd_buffer[write_idx] = keymap_us[shift_pressed][scancode];
                write_idx = (write_idx + 1) % KBD_BUF_SIZE;
            }
        }
    }
}

int keyboard_getch()
{
    uint8_t c;

    while (read_idx == write_idx) ;
    c = kbd_buffer[read_idx];
    read_idx = (read_idx + 1) % KBD_BUF_SIZE;
    return c;
}

void keyboard_init()
{
    attach_interrupt_handler(IRQ(IRQ_KBD), keyboard_handler);
    pic_enable_irq(IRQ_KBD);
}
