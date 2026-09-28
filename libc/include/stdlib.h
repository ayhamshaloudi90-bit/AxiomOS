#ifndef AXIOM_LIBC_STDLIB_H
#define AXIOM_LIBC_STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);
int atoi(const char *text);
long strtol(const char *text, char **endptr, int base);

#endif
