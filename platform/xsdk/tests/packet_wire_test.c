// Forces the big-endian path on any host: the bytes on the wire must match a little-endian build's.
#include <assert.h>
#include <stdint.h>
#include <string.h>
#define PACKET_WIRE_SWAP 1
#include "packet_wire.h"

int main(void) {
    uint8_t value[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    const uint8_t two[8] = { 2, 1, 3, 4, 5, 6, 7, 8 };
    packet_wire_order(value, 2);
    assert(memcmp(value, two, 8) == 0);
    packet_wire_order(value, 2);
    const uint8_t four[8] = { 4, 3, 2, 1, 5, 6, 7, 8 };
    packet_wire_order(value, 4);
    assert(memcmp(value, four, 8) == 0);
    packet_wire_order(value, 4);
    const uint8_t eight[8] = { 8, 7, 6, 5, 4, 3, 2, 1 };
    packet_wire_order(value, 8);
    assert(memcmp(value, eight, 8) == 0);
    assert(packet_wire_is_scalar(2) && packet_wire_is_scalar(4) && packet_wire_is_scalar(8));
    assert(!packet_wire_is_scalar(0) && !packet_wire_is_scalar(1) && !packet_wire_is_scalar(3) && !packet_wire_is_scalar(24));
    return 0;
}
