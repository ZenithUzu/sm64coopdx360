#ifndef PACKET_WIRE_H
#define PACKET_WIRE_H

#include <stdint.h>

// The wire is little-endian, as the PC and phone builds write it.
#ifndef PACKET_WIRE_SWAP
#define PACKET_WIRE_SWAP (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#endif

// Converts one scalar between native and wire order, in either direction.
static inline void packet_wire_order(void* data, uint16_t length) {
    if (!PACKET_WIRE_SWAP) { return; }
    uint8_t* bytes = (uint8_t*)data;
    for (uint16_t i = 0; i < length / 2; i++) {
        uint8_t byte = bytes[i];
        bytes[i] = bytes[length - 1 - i];
        bytes[length - 1 - i] = byte;
    }
}

static inline int packet_wire_is_scalar(uint16_t length) {
    return length == 2 || length == 4 || length == 8;
}

#endif
