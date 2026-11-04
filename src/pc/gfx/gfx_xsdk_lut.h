#ifndef GFX_XSDK_LUT_H
#define GFX_XSDK_LUT_H
#include <math.h>
#include <stdbool.h>

// One channel of the display LUT, 8 bits in and 10 out: gamma in percent (100 is linear), squeezed to video levels when limited.
static inline unsigned gfx_xsdk_lut_value(unsigned index, unsigned gamma_percent, bool limited) {
    const double low = limited ? 64.0 : 0.0, high = limited ? 940.0 : 1023.0;
    return (unsigned)(low + pow(index / 255.0, gamma_percent / 100.0) * (high - low) + 0.5);
}
#endif
