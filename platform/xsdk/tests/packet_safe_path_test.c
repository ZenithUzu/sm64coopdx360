#include <assert.h>
#include <stdio.h>
#include "packet_safe_path.h"

int main(void) {
    assert(packet_path_is_safe("mod.lua"));
    assert(packet_path_is_safe("[CS] Pack/sound/a..b.m64"));
    assert(packet_path_is_safe("folder\\file...lua"));
    assert(!packet_path_is_safe(".."));
    assert(!packet_path_is_safe("../x.lua"));
    assert(!packet_path_is_safe("a/../../x.lua"));
    assert(!packet_path_is_safe("a\\..\\x.lua"));
    assert(!packet_path_is_safe("a/.."));
    assert(!packet_path_is_safe("/etc/x"));
    assert(!packet_path_is_safe("\\Windows\\x"));
    assert(!packet_path_is_safe("C:\\Users\\x"));
    assert(!packet_path_is_safe("Hdd1:/x"));
    puts("packet_safe_path_test: ok");
    return 0;
}
