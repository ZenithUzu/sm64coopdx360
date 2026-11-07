#ifndef FATX_NAMES_H
#define FATX_NAMES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// FATX keeps names of 42 characters at most. Longer ones are stored under a short name, kept unique by a hash of the
// long one, and each folder's FATX_NAMES_INDEX lists "short<TAB>long" lines so listings can give the long name back.
enum { FATX_NAME_MAX = 42 };
#define FATX_NAMES_INDEX "_longnames.txt"

static inline uint32_t fatx_name_hash(const char *name, size_t length) {
    uint32_t hash = 2166136261u;  // FNV-1a
    for (size_t i = 0; i < length; i++) { hash = (hash ^ (uint8_t)name[i]) * 16777619u; }
    return hash;
}

// The stored form of one name: unchanged when it fits; else its start, '~', 8 hex digits and its extension.
static inline void fatx_short_name(const char *name, size_t length, char out[FATX_NAME_MAX + 1]) {
    if (length <= FATX_NAME_MAX) { memcpy(out, name, length); out[length] = '\0'; return; }
    size_t ext = 0;
    for (size_t i = length; i > 0 && length - i < 8; i--) {
        if (name[i - 1] == '.') { ext = length - (i - 1); break; }
    }
    const size_t keep = FATX_NAME_MAX - 9 - ext;
    static const char hex[] = "0123456789abcdef";
    const uint32_t hash = fatx_name_hash(name, length);
    memcpy(out, name, keep);
    out[keep] = '~';
    for (int i = 0; i < 8; i++) { out[keep + 1 + i] = hex[(hash >> (28 - 4 * i)) & 15]; }
    memcpy(out + keep + 9, name + length - ext, ext);
    out[FATX_NAME_MAX] = '\0';
}

// Writes path with every long component in its stored form; false if nothing changed or out is too small.
static inline bool fatx_short_path(const char *path, char *out, size_t size) {
    bool changed = false;
    size_t o = 0;
    for (const char *p = path; *p;) {
        if (*p == '/' || *p == '\\') {
            if (o + 1 >= size) { return false; }
            out[o++] = *p++;
            continue;
        }
        size_t length = strcspn(p, "/\\");
        char name[FATX_NAME_MAX + 1];
        const char *part = p;
        if (length > FATX_NAME_MAX) { fatx_short_name(p, length, name); part = name; changed = true; }
        const size_t written = length > FATX_NAME_MAX ? FATX_NAME_MAX : length;
        if (o + written >= size) { return false; }
        memcpy(out + o, part, written);
        o += written;
        p += length;
    }
    out[o] = '\0';
    return changed;
}

#endif
