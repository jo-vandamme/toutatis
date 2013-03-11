#ifndef __KERNEL_LOGGING_H__
#define __KERNEL_LOGGING_H__

#include <stdarg.h>

typedef enum
{
        DEBUG = 0,      /* debug information */
        INFO,           /* unimportant */
        NOTICE,         /* important, but not bad */
        WARNING,        /* not what was expected, but still okay */
        ERROR,          /* this is bad... */
        CRITICAL        /* fatal error */
} log_level_t;

void logging_init();

int kprintf(log_level_t level, const char *fmt, ...);

#endif
