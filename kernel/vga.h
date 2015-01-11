#ifndef __KERNEL_VGA_H__
#define __KERNEL_VGA_H__

#include <types.h>
#include "driver.h"

#define COLS    80
#define ROWS    25

device_t *vga_init();
size_t vga_write(uint8_t *data, size_t len);

void vga_clear();
void vga_scroll();
void vga_print_char(const char c);
void vga_print_str(const char *str);
void vga_print_dec(const uint32_t value);
void vga_print_hex(const uint32_t value);
void vga_set_attribute(const uint16_t att);
void get_cursor_pos(uint16_t *x, uint16_t *y);
void set_cursor_pos(uint16_t x, uint16_t y);
void set_pos(uint16_t x, uint16_t y);

#endif
