#ifndef __KERNEL_UTILS_H__
#define __KERNEL_UTILS_H__

#include <types.h>

#define is_upper(c)  ((c) >= 'A' && (c) <= 'Z')
#define to_lower(c)  (is_upper(c) ? (((c) - 'A') + 'a') : (c))
#define is_alpha(c)  (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define is_digit(c)  ((c) >= '0' && (c) <= '9')
#define is_alnum(c)  (is_alpha(c) || is_digit(c))
#define is_xdigit(c) (is_digit(c) || ((c) > 'a' && (c) < 'f'))

long int strtol(const char *str, char **endptr, int base);
char *itoa(unsigned long value, char *str, int base);
int   atoi(char *str);

#endif
