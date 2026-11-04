#ifndef DYNOS_TEX_SHRINK_H
#define DYNOS_TEX_SHRINK_H

#include <stdint.h>

// Halves an RGBA image in place, averaging each 2x2 block, until both sides fit in maxSide.
static inline void dynos_tex_shrink(uint8_t *rgba, int *width, int *height, int maxSide) {
    while (*width > maxSide || *height > maxSide) {
        const int w = *width, h = *height;
        const int nw = w > 1 ? w / 2 : 1, nh = h > 1 ? h / 2 : 1;
        for (int y = 0; y < nh; y++) {
            const int y0 = y * h / nh, y1 = h > 1 ? y0 + 1 : y0;
            for (int x = 0; x < nw; x++) {
                const int x0 = x * w / nw, x1 = w > 1 ? x0 + 1 : x0;
                for (int c = 0; c < 4; c++) {
                    const int sum = rgba[(y0 * w + x0) * 4 + c] + rgba[(y0 * w + x1) * 4 + c] +
                                    rgba[(y1 * w + x0) * 4 + c] + rgba[(y1 * w + x1) * 4 + c];
                    rgba[(y * nw + x) * 4 + c] = (uint8_t)((sum + 2) / 4);
                }
            }
        }
        *width = nw;
        *height = nh;
    }
}

#endif
