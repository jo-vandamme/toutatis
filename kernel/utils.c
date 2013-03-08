#include "utils.h"
#include "vga.h"

long int strtol(const char *str, char **endptr, int base)
{
        const char *buf = str;
        long int value = 0;
        int sign = 1, k = 0;

        if (base < 2 || base > 36) {
                return 0;
        }

        /* swallow white spaces */
        while (*buf == ' ' || *buf == '\t') {
                ++buf;
        }

        /* parse sign if any */
        if (*buf == '-') {
                sign = -1;
                ++buf;
        } else if (*buf == '+') {
                sign = 1;
                ++buf;
        }

        /* parse base */
        if (base == 0) {
                if (*buf == '0') {
                        if (to_lower(*(++buf)) == 'x' && is_xdigit(buf[1])) {
                                ++buf;
                                base = 16;
                        } else {
                                base = 8;
                        }
                } else {
                        base = 10;
                }
        } else if (base == 16 && buf[0] == '0' && to_lower(buf[1]) == 'x') {
                str += 2;
        }

        /* parse alpha-numerical string */
        while (is_alnum(*buf)) {
                if (is_alpha(*buf)) {
                        k = to_lower(*buf) - 'a' + 10;
                        if (k > base) {
                                break;
                        }
                } else {
                        k = *buf - '0';
                }
                value = value * base + k;
                ++buf;
        }

        if (endptr != NULL) {
                *endptr = (char *)buf;
        }

        return sign * value;
}

char *itoa(unsigned long value, char *str, int base)
{
        char *ptr, *low;

        // Check for supported base (2 to 36)
        if (base < 2 || base > 36) {
                *str = 0;
                return str;
        }
        ptr = low = str;

        do {
                *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[value % base];
                value /= base;
        } while (value);
        *ptr-- = 0;

        // Invert the numbers.
        while (low < ptr) {
                char tmp = *low;
                *low++ = *ptr;
                *ptr-- = tmp;
        }
        return str;
}

int atoi(char *str)
{
        int k, n = 0;
        int base = 10;

        while (*str == ' ') {
                ++str;
        }
        if (*str == '0') {
                base = 8;
                ++str;
                if (*str == 'x') {
                        base = 16;
                        ++str;
                }
        }

        while (*str) {
                if (*str == ' ') {
                        break;
                }
                if (is_alpha(*str)) {
                        k = to_lower(*str) - 'a' + 10;
                } else {
                        k = *str - '0';
                }
                n = n * base + k;
                ++str;
        }

        return n;
}
