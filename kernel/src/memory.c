#include <memory.h>
#include <stddef.h>
#include <stdint.h>

// GCC and Clang may emit calls to these four standard routines even when KBM
// never calls them explicitly. A freestanding kernel supplies its own versions.

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    // memcpy assumes the source and destination ranges do not overlap.
    uint8_t *restrict pdest = dest;
    const uint8_t *restrict psrc = src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    // Only the low byte of c is repeated through the destination range.
    uint8_t *p = s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    // Unlike memcpy, memmove preserves data when the two ranges overlap.
    uint8_t *pdest = dest;
    const uint8_t *psrc = src;

    if ((uintptr_t)src > (uintptr_t)dest) {
        // Forward copying is safe when the source starts after the destination.
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if ((uintptr_t)src < (uintptr_t)dest) {
        // Copy backward whenever the destination starts at a higher address.
        for (size_t i = n; i > 0; i--) {
            pdest[i - 1] = psrc[i - 1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    // Return only ordering information; callers must not depend on an exact value.
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}
