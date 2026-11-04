#include <assert.h>
#include <stdio.h>
#include "gfx_xsdk_lut.h"

int main(void) {
    assert(gfx_xsdk_lut_value(0, 100, false) == 0);
    assert(gfx_xsdk_lut_value(255, 100, false) == 1023);
    assert(gfx_xsdk_lut_value(128, 100, false) == 514);
    assert(gfx_xsdk_lut_value(0, 100, true) == 64);
    assert(gfx_xsdk_lut_value(255, 100, true) == 940);
    // Below 100 lifts the mids, above lowers them, and the ends stay put.
    assert(gfx_xsdk_lut_value(128, 80, false) > 514);
    assert(gfx_xsdk_lut_value(128, 120, false) < 514);
    assert(gfx_xsdk_lut_value(255, 70, false) == 1023);
    for (unsigned gamma = 70; gamma <= 130; gamma++) {
        for (unsigned i = 1; i < 256; i++) {
            assert(gfx_xsdk_lut_value(i, gamma, true) >= gfx_xsdk_lut_value(i - 1, gamma, true));
        }
    }
    puts("gfx_xsdk_lut_test: ok");
    return 0;
}
