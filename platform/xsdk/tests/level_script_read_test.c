#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <PR/mbi.h>
#include "level_commands.h"
#include "level_script_read.h"

static const uintptr_t destination[] = { RETURN() };
static const uintptr_t jump[] = { JUMP_LINK(destination), RETURN() };
static const uintptr_t area[] = { AREA(7, destination) };
static const uintptr_t position[] = { MARIO_POS(3, -90, -123, 222, -456) };
static const uintptr_t warp[] = { WARP_NODE(11, 9, 2, 3, 0) };

int main(void) {
    assert(level_script_read_pointer(jump, 4) == (uintptr_t)destination);
    assert(level_script_read_pointer(area, 4) == (uintptr_t)destination);
    assert(level_script_read_field(area, 2, 1) == 7);
    assert(level_script_read_field(position, 2, 1) == 3);
    assert((int16_t)level_script_read_field(position, 4, 2) == -90);
    assert((int16_t)level_script_read_field(position, 6, 2) == -123);
    assert((int16_t)level_script_read_field(position, 8, 2) == 222);
    assert((int16_t)level_script_read_field(position, 10, 2) == -456);
    assert(level_script_read_field(warp, 2, 1) == 11);
    assert(level_script_read_field(warp, 3, 1) == 9);
    assert(level_script_read_field(warp, 4, 1) == 2);
    assert(level_script_read_field(warp, 5, 1) == 3);
    puts("PASS: actual JUMP_LINK, AREA, MARIO_POS and WARP_NODE command fields");
    return 0;
}

