#ifndef __KERNEL_STRING_H__
#define __KERNEL_STRING_H__

#include <types.h>

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *ptr, int value, size_t n);
void *memsetw(void *ptr, unsigned short value, size_t n);
void *memmove(void *dst, const void *src, size_t n);
int memcmp(const void *ptr1, const void *ptr2, size_t n);
void *memchr(const void *ptr, int value, size_t n);

size_t strlen(const char *str);
size_t strnlen(const char *str, size_t maxlen);

char *strcat(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);

#endif
