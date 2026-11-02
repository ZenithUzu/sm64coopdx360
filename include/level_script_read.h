#ifndef LEVEL_SCRIPT_READ_H
#define LEVEL_SCRIPT_READ_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Compiled command fields are native-endian. Four-byte script slots become
 * pointer-sized slots on a 64-bit host; byte offsets within a slot stay put. */
static inline size_t level_script_field_offset(size_t offset) {
    return (offset & 3) | ((offset & ~(size_t)3) * (sizeof(uintptr_t) / 4));
}

static inline uint64_t level_script_read_field(const void *command, size_t offset, size_t bytes) {
    const unsigned char *source = (const unsigned char *)command + level_script_field_offset(offset);
    switch (bytes) {
        case 1: { uint8_t value; memcpy(&value, source, sizeof value); return value; }
        case 2: { uint16_t value; memcpy(&value, source, sizeof value); return value; }
        case 4: { uint32_t value; memcpy(&value, source, sizeof value); return value; }
        case 8: { uint64_t value; memcpy(&value, source, sizeof value); return value; }
        default: return 0;
    }
}

static inline uintptr_t level_script_read_pointer(const void *command, size_t offset) {
    return (uintptr_t)level_script_read_field(command, offset, sizeof(uintptr_t));
}

#endif
