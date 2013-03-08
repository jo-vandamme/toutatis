#ifndef __KERNEL_VARGS_H__
#define __KERNEL_VARGS_H__

/* width of stack = 4 bytes */
#define STACKITEM int

/* data type used for arguments */
#define va_list char *

/* Amount of space required on the stack for an arg of type TYPE.
   TYPE may alternatively be an expression whose type is used. */
#define __va_rounded_size(TYPE) \
        ((sizeof(TYPE) + sizeof(STACKITEM) - 1) & ~(sizeof(STACKITEM) - 1))

/* initialize AP so that it points to the first argument (right after LASTARG) */
#define va_start(AP, LASTARG) \
        (AP = ((va_list) &(LASTARG) + __va_rounded_size(LASTARG)))

/* do nothing */
#define va_end(AP)

/* return the next argument in the argument list and increment AP */
#define va_arg(AP, TYPE) \
        (AP += __va_rounded_size(TYPE), *((TYPE *)(AP - __va_rounded_size(TYPE))))

#endif
