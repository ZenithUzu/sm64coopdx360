// Little-endian command words, as PC builds write them, must come out as the big-endian macros in
// include/command_macros_base.h pack them.
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#define PACKET_WIRE_SWAP 1
#include "dynos_cmd_order.h"

#define LE_BBH(a, b, c) ((uint32_t)(a) | (uint32_t)(b) << 8 | (uint32_t)(c) << 16)
#define LE_BBBB(a, b, c, d) ((uint32_t)(a) | (uint32_t)(b) << 8 | (uint32_t)(c) << 16 | (uint32_t)(d) << 24)
#define LE_HH(a, b) ((uint32_t)(a) | (uint32_t)(b) << 16)
#define BE_BBH(a, b, c) ((uint32_t)(a) << 24 | (uint32_t)(b) << 16 | (uint32_t)(c))
#define BE_BBBB(a, b, c, d) ((uint32_t)(a) << 24 | (uint32_t)(b) << 16 | (uint32_t)(c) << 8 | (uint32_t)(d))
#define BE_HH(a, b) ((uint32_t)(a) << 16 | (uint32_t)(b))

int main(void) {
    // INSTANT_WARP: BBBB, then two HH.
    struct DynosCmdOrder level = { NULL };
    assert(dynos_cmd_order_word(&level, LE_BBBB(0x28, 0x0C, 1, 2), 1) == BE_BBBB(0x28, 0x0C, 1, 2));
    assert(dynos_cmd_order_word(&level, LE_HH(0x1234, 0xABCD), 1) == BE_HH(0x1234, 0xABCD));
    assert(dynos_cmd_order_word(&level, LE_HH(0x5678, 0), 1) == BE_HH(0x5678, 0));
    // JUMP: BBH, then a value that stays as it is; the next word starts a new command.
    assert(dynos_cmd_order_word(&level, LE_BBH(0x05, 0x08, 0), 1) == BE_BBH(0x05, 0x08, 0));
    assert(dynos_cmd_order_word(&level, 0xDEADBEEF, 1) == 0xDEADBEEF);
    assert(dynos_cmd_order_word(&level, LE_BBH(0x02, 0x04, 0), 1) == BE_BBH(0x02, 0x04, 0));

    // GEO_TRANSLATE_WITH_DL: the flag in the layer byte adds the pointer.
    struct DynosCmdOrder geo = { NULL };
    assert(dynos_cmd_order_word(&geo, LE_BBH(0x11, 0x81, 0x4321), 0) == BE_BBH(0x11, 0x81, 0x4321));
    assert(dynos_cmd_order_word(&geo, LE_HH(1, 2), 0) == BE_HH(1, 2));
    assert(dynos_cmd_order_word(&geo, 0xCAFEF00D, 0) == 0xCAFEF00D);
    assert(dynos_cmd_order_word(&geo, LE_BBH(0x01, 0, 0), 0) == BE_BBH(0x01, 0, 0));
    return 0;
}
