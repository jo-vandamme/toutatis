#include <system.h>
#include <logging.h>
#include <driver.h>
#include <stdarg.h>
#include <vsprintf.h>
#include <string.h>

static device_t *vga_driver;
static device_t *com_driver;

static uint8_t volatile log_lock = 0;

void logging_init(device_t *vga, device_t *com)
{
        vga_driver = vga;
        com_driver = com;
}

int kprintf(log_level_t level, const char *fmt, ...)
{
        char buf[1024];
        va_list args;
        int n = 0;

        va_start(args, fmt);
        n = vsprintf(buf, fmt, args);
        va_end(args);

        //spin_lock(&log_lock);
        
        com_driver->write((uint8_t *)buf, strlen(buf));

        if (level >= INFO) {
            vga_driver->write((uint8_t *)buf, strlen(buf));
        }

        //spin_unlock(&log_lock);

        return n;
}
