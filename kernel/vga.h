#ifndef __KERNEL_VGA_H__
#define __KERNEL_VGA_H__

#include <types.h>
#include "driver.h"

#define COLS    80
#define ROWS    25

#define info(str)   vga_print_str("[[0x0e]]->[[0x07]] " str "\n")
#define error(str)  vga_print_str("[[0x0c]][ERROR][[0x04]] " str "\n")
#define newline     vga_print_str("\n")

device_t *vga_init();
size_t vga_write(u8_t *data, size_t len);

void vga_clear();
void vga_scroll();
void vga_print_char(const char c);
void vga_print_str(const char *str);
void vga_print_dec(const u32_t value);
void vga_print_hex(const u32_t value);
void vga_set_attribute(const u16_t att);
void get_cursor_pos(u16_t *x, u16_t *y);
void set_cursor_pos(u16_t x, u16_t y);

#endif
