#include <string.h>
#include <utils.h>
#include "vga.h"
#include "arch.h"
#include "driver.h"

#define CURS_CTRL       0x3d4
#define CURS_DATA       0x3d5
#define HIGH_BYTE       14
#define LOW_BYTE        15

static u16_t xpos = 0, ypos = 0;
static u16_t *video_mem = (u16_t *)0xb8000;
static u16_t attribute = 0x0700;
static device_t vga_device;

device_t *vga_init()
{
        get_cursor_pos(&xpos, &ypos);
        vga_clear();

        vga_device.read = 0;
        vga_device.write = vga_write;

        return &vga_device;
}

void vga_clear()
{
        u16_t i;
        for (i = 0; i < COLS * ROWS; ++i) {
                video_mem[i] = (u8_t)' ' | attribute;
        }
        xpos = ypos = 0;
        set_cursor_pos(0, 0);
}

void vga_print_char(const char c)
{
        /* backspace */
        if (c == '\b') {
                if (xpos > 0) {
                        --xpos;
                } else if (ypos > 0) {
                        xpos = COLS - 1;
                        --ypos;
                }
        }
        /* tab -> increment xpos to a point that will make it
         * divisible by 8. */
        else if (c == '\t') {
                xpos = (xpos + 8) & ~(8 - 1);
        }
        /* carriage return */
        else if (c == '\r') {
                xpos = 0;
        }
        /* newline */
        else if (c == '\n') {
                xpos = 0;
                ++ypos;
        }
        /* any character greater than and including a space is
         * a printable character */
        else if (c >= ' ') {
                u16_t *pos = video_mem + (ypos * COLS + xpos);
                *pos = (u16_t)c | attribute;
                ++xpos;
        }

        if (xpos >= COLS) {
                xpos = 0;
                ++ypos;
        }

        vga_scroll();
        set_cursor_pos(xpos, ypos);
}

void vga_print_dec(const u32_t value)
{
        int i = 0;
        char buffer[12];

        itoa(value, buffer, 10);
        while (buffer[i]) {
                vga_print_char(buffer[i++]);
        }
}

void vga_print_hex(const u32_t value)
{
        int i = 0;
        char buffer[12];

        itoa(value, buffer + 0, 16);
        while (buffer[i]) {
                vga_print_char(buffer[i++]);
        }
}

size_t vga_write(u8_t *data, size_t len)
{
        size_t i;

        for (i = 0; *data && i < len; ++data, ++i) {
                if (*data == '\033') {
                        ++data;
                        ++i;
                        vga_set_attribute(*data << 8);
                } else {
                        vga_print_char((const char)*data);
                }
        }
        return i;
}

void vga_print_str(const char *str)
{
        while (*str) {
                /* detect color attributes like \033\007 */
                if (*str == '\033') {
                        vga_set_attribute(*++str << 8);
                        ++str;
                } else {
                        vga_print_char(*str++);
                }
        }
}

void vga_set_attribute(const u16_t attr)
{
        attribute = attr;
}

void vga_scroll()
{
        u16_t blank, temp;

        if (ypos >= ROWS) {
                /* move the current text chunk back in the buffer by a line */
                temp = ypos - ROWS + 1; /* points to the start of the chunk to be moved */
                memcpy(video_mem, video_mem + temp * COLS, (ROWS - temp) * COLS * 2);

                /* finally, clear the last line */
                blank = (u8_t)' ' | attribute;
                memsetw(video_mem + (ROWS - temp) * COLS, blank, COLS);
                ypos = ROWS - 1;
        }
}

void get_cursor_pos(u16_t *x, u16_t *y)
{
        u16_t pos;

        outb(CURS_CTRL, HIGH_BYTE);
        pos = inb(CURS_DATA) << 8;

        outb(CURS_CTRL, LOW_BYTE);
        pos |= inb(CURS_DATA);

        *x = pos % COLS;
        *y = pos / COLS;
}

void set_cursor_pos(u16_t x, u16_t y)
{
        u16_t pos = y * COLS + x;

        outb(CURS_CTRL, HIGH_BYTE);
        outb(CURS_DATA, pos >> 8);

        outb(CURS_CTRL, LOW_BYTE);
        outb(CURS_DATA, pos);
}
