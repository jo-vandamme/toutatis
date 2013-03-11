#include <logging.h>
#include <driver.h>
#include <stdarg.h>
#include <vsprintf.h>
#include <string.h>

static device_t *vga_driver;
static device_t *com_driver;

void logging_init(device_t *vga, device_t *com)
{
        vga_driver = vga;
        com_driver = com;
}

int kprintf(const char *fmt, ...)
{
        char buf[1024];
        va_list args;
        int n = 0;

        va_start(args, fmt);
        n = vsprintf(buf, fmt, args);
        va_end(args);

        vga_driver->write((u8_t *)buf, strlen(buf));
        com_driver->write((u8_t *)buf, strlen(buf) - 1);

        return n;
}
