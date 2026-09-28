#ifndef AXIOM_LIBC_STDIO_H
#define AXIOM_LIBC_STDIO_H

#include <stddef.h>
#include <stdarg.h>

int putchar(int c);
int puts(const char *s);
int printf(const char *format, ...);
int snprintf(char *buffer, size_t capacity, const char *format, ...);
int vsnprintf(char *buffer, size_t capacity, const char *format, va_list args);

#endif
