#include <stddef.h>
#include <stdint.h>

/*
 * Minimal freestanding memory primitives for the kernel.
 *
 * These symbols intentionally use the standard ABI names because Clang/LLVM
 * may lower aggregate copies/initialization to memcpy/memset/memmove even in
 * a freestanding build.  Volatile byte accesses keep the implementations from
 * being folded back into calls to themselves at -O2.
 */

void *memcpy(void *destination, const void *source, size_t count)
{
    volatile unsigned char *dst = (volatile unsigned char *)destination;
    const volatile unsigned char *src =
        (const volatile unsigned char *)source;
    size_t index;

    for (index = 0u; index < count; ++index) {
        dst[index] = src[index];
    }

    return destination;
}

void *memset(void *destination, int value, size_t count)
{
    volatile unsigned char *dst = (volatile unsigned char *)destination;
    const unsigned char byte = (unsigned char)value;
    size_t index;

    for (index = 0u; index < count; ++index) {
        dst[index] = byte;
    }

    return destination;
}

void *memmove(void *destination, const void *source, size_t count)
{
    volatile unsigned char *dst = (volatile unsigned char *)destination;
    const volatile unsigned char *src =
        (const volatile unsigned char *)source;

    if (dst == src || count == 0u) {
        return destination;
    }

    if ((uintptr_t)dst < (uintptr_t)src) {
        size_t index;

        for (index = 0u; index < count; ++index) {
            dst[index] = src[index];
        }
    } else {
        size_t index = count;

        while (index != 0u) {
            --index;
            dst[index] = src[index];
        }
    }

    return destination;
}

int memcmp(const void *left, const void *right, size_t count)
{
    const volatile unsigned char *lhs =
        (const volatile unsigned char *)left;
    const volatile unsigned char *rhs =
        (const volatile unsigned char *)right;
    size_t index;

    for (index = 0u; index < count; ++index) {
        if (lhs[index] != rhs[index]) {
            return lhs[index] < rhs[index] ? -1 : 1;
        }
    }

    return 0;
}
