#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "dynos_tex_shrink.h"

int main(void) {
    // 4x2 with a side limit of 2: one halving, each block averaged per channel.
    uint8_t image[4 * 2 * 4];
    memset(image, 0, sizeof image);
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 4; x++) {
            image[(y * 4 + x) * 4] = x < 2 ? 100 : 200;
            image[(y * 4 + x) * 4 + 3] = (x == 0 && y == 0) ? 0 : 255;
        }
    }
    int width = 4, height = 2;
    dynos_tex_shrink(image, &width, &height, 2);
    assert(width == 2 && height == 1);
    assert(image[0] == 100 && image[4] == 200);
    assert(image[3] == 191 && image[7] == 255);

    // Already small enough: untouched.
    uint8_t pixel[4] = { 1, 2, 3, 4 };
    width = height = 1;
    dynos_tex_shrink(pixel, &width, &height, 512);
    assert(width == 1 && height == 1 && pixel[0] == 1 && pixel[3] == 4);

    // A long thin image keeps at least one row.
    uint8_t thin[8 * 4];
    memset(thin, 40, sizeof thin);
    width = 8; height = 1;
    dynos_tex_shrink(thin, &width, &height, 2);
    assert(width == 2 && height == 1 && thin[0] == 40);
    return 0;
}
