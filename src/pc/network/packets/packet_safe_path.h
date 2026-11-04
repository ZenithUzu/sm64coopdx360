#ifndef PACKET_SAFE_PATH_H
#define PACKET_SAFE_PATH_H

#include <stdbool.h>

// A mod path from another player must stay inside the downloads folder: not absolute, no drive, no ".." segment.
static inline bool packet_path_is_safe(const char* path) {
    if (path[0] == '/' || path[0] == '\\') { return false; }
    const char* segment = path;
    for (const char* c = path; ; c++) {
        if (*c == ':') { return false; }
        if (*c == '/' || *c == '\\' || *c == '\0') {
            if (c - segment == 2 && segment[0] == '.' && segment[1] == '.') { return false; }
            if (*c == '\0') { return true; }
            segment = c + 1;
        }
    }
}

#endif
