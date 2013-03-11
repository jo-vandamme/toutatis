#ifndef __KERNEL_KPRINTF_H__
#define __KERNEL_KPRINTF_H__

#include <stdarg.h>

int vsprintf(char *s, const char *fmt, va_list arg);
int sprintf(char *buf, const char *fmt, ...);

#endif
