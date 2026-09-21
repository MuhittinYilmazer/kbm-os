#ifndef KBM_KERNEL_FONT_H
#define KBM_KERNEL_FONT_H

#include <stdint.h>

// Return an 8x8 bitmap for KBM's supported ASCII, punctuation, and Turkish glyph subset.
const uint8_t *font_get_glyph(uint32_t character);

#endif
