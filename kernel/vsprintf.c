#include <vsprintf.h>
#include <types.h>
#include <stdarg.h>
#include <string.h>
#include <utils.h>
#include <vga.h>

#define LEFT    (1 << 0)
#define PLUS    (1 << 1)
#define SIGNED  (1 << 2)
#define SPACE   (1 << 3)
#define PREFIX  (1 << 4)
#define ZEROS   (1 << 5)
#define LARGE   (1 << 6)

static const char digits_lower[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char digits_upper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static unsigned number(char *str, unsigned long num, unsigned base, int width, int precision, unsigned flags)
{
        char buffer[64] = { 0 };
        char sign = 0, pad;
        int total = 0, i;
        unsigned res;

        if (flags & LEFT) {
                flags &= ~ZEROS;
        }

        if (base < 2 || base > 36) {
                return 0;
        }

        pad = (flags & ZEROS) ? '0' : ' ';

        /* work out the sign character to use, if any. Always print a
         * sign character if the number is negative. */
        if (flags & SIGNED) {
                if ((int)num < 0) {
                        num = -num;
                        sign = '-';
                        --width;
                } else if (flags & PLUS) {
                        sign = '+';
                        --width;
                } else if (flags & SPACE) {
                        sign = ' ';
                        --width;
                }
        }

        /* reduce field to accomodate any prefixed required. */
        if (flags & PREFIX) {
                if (base == 16) {
                        width -= 2;
                } else if (base == 8) {
                        --width;
                }
        }

        /* write the number in reverse order to the temporary buffer. */
        i = 0;
        if (num == 0) {
                buffer[i++] = '0';
        } else {
                while (num != 0) {
                        res = (unsigned)(num % base);
                        num /= base;
                        buffer[i++] = (flags & LARGE) ? digits_upper[res] : digits_lower[res];
                }
        }

        /* the precision is the minimum number of digits to print,
         * so if the digit is higher than the precision, set precision
         * to the digit count. Width then becomes the amount of padding
         * we require. */
        if (i > precision) {
                precision = i;
        }
        width -= precision;

        /* we are not left aligned and require space padding. */
        if (!(flags & (ZEROS | LEFT))) {
                while (width-- > 0) {
                        *str++ = ' ';
                        ++total;
                }
        }

        /* if there is a sign, write it. */
        if (sign) {
                *str++ = sign;
                ++total;
        }

        /* if there is a prefix, write it. */
        if (flags & PREFIX) {
                if (base == 8) {
                        *str++ = '0';
                        ++total;
                } else if (base == 16) {
                        *str++ = '0';
                        *str++ = (flags & LARGE) ? 'X' : 'x';
                        total += 2;
                }
        }

        /* zero padding. */
        if (!(flags & LEFT)) {
                while (width-- > 0) {
                        *str++ = pad;
                        ++total;
                }
        }
        while (i < precision--) {
                *str++ = '0';
                ++total;
        }

        /* write number, reversed to correct direction. */
        while (i-- > 0) {
                *str++ = buffer[i];
                ++total;
        }

        /* left padding for left justification. */
        while (width-- > 0) {
                *str++ = ' ';
                ++total;
        }

        return total - 1;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
        unsigned loc = 0, i, k;
        unsigned long val, flags;
        char tmp[1024] = { 0 };
        int width, precision;

        if (!buf || !fmt)
                return 0;

        for (i = 0; i <= strlen(fmt); ++i, ++loc) {
                switch (fmt[i]) {
                case '%':
                        /* process the flags */
                        flags = 0;
                        for (;;) {
                                switch (fmt[++i]) {
                                case '#':
                                        flags |= PREFIX;
                                        continue;
                                case '0':
                                        /* LEFT has higher precedence */
                                        if (!(flags & LEFT)) {
                                                flags |= ZEROS;
                                        }
                                        continue;
                                case '-':
                                        flags &= ~ZEROS;
                                        flags |= LEFT;
                                        continue;
                                case '+':
                                        flags |= PLUS;
                                        continue;
                                case ' ':
                                        flags &= ~PLUS;
                                        flags |= SPACE;
                                        continue;
                                default:
                                        --i;
                                        break;
                                }
                                break;
                        }

                        /* get the field width */
                        width = -1;
                        if (is_digit(fmt[i+1])) {
                                for (k = 0; is_digit(fmt[i+1]); ++k) {
                                        tmp[k] = fmt[++i];
                                }
                                tmp[k] = 0;
                                width = atoi(tmp);
                        } else if (fmt[i+1] == '*') {
                                width = va_arg(args, int);
                                ++i;
                                if (width < 0) {
                                        width = -width;
                                        flags |= LEFT;
                                }
                        }

                        /* get the precision */
                        precision = -1;
                        if (fmt[i+1] == '.') {
                                ++i;
                                if (is_digit(fmt[i+1])) {
                                        for (k = 0; is_digit(fmt[i+1]); ++k) {
                                                tmp[k] = fmt[++i];
                                        }
                                        tmp[k] = 0;
                                        precision = atoi(tmp);
                                } else if (fmt[i+1] == '*') {
                                        precision = va_arg(args, int);
                                        ++i;
                                }
                                if (precision < 0) {
                                        precision = 0;
                                }
                        }

                        /* get the length modifier */

                        /* get the conversion specifier */
                        switch (fmt[i+1]) {
                        case 'c':
                                val = va_arg(args, char);
                                buf[loc] = val;
                                ++i;
                                break;
                        case 's':
                                val = (unsigned long)va_arg(args, const char *);
                                strncpy(tmp, (const char *)val, 1024);
                                strncpy(&buf[loc], tmp, 1024);
                                loc += strlen(tmp) - 2;
                                ++i;
                                break;
                        case 'd':
                        case 'i':
                                flags |= SIGNED;
                                val = va_arg(args, int);
                                loc += number(&buf[loc], val, 10, width, precision, flags);
                                ++i;
                                break;
                        case 'u':
                                val = va_arg(args, unsigned int);
                                loc += number(&buf[loc], val, 10, width, precision, flags);
                                ++i;
                                break;
                        case 'o':
                                val = va_arg(args, unsigned long);
                                loc += number(&buf[loc], val, 8, width, precision, flags);
                                ++i;
                                break;
                        case 'X':
                                flags |= LARGE;
                        case 'x':
                                val = va_arg(args, unsigned long);
                                loc += number(&buf[loc], val, 16, width, precision, flags);
                                ++i;
                                break;
                        case 'b':
                                val = va_arg(args, unsigned long);
                                loc += number(&buf[loc], val, 2, width, precision, flags);
                                ++i;
                                break;
                        }
                        break;
                default:
                        buf[loc] = fmt[i];
                        break;
                }
        }

        return i;
}

int sprintf(char *buf, const char *fmt, ...)
{
        int len = 0;
        va_list args;
        va_start(args, fmt);
        len = vsprintf(buf, fmt, args);
        va_end(args);
        return len;
}
