// Long file names on FATX: the linker sends the game's file calls here (--wrap, platform/xsdk/CMakeLists.txt), which
// store long names under short ones and give the long ones back in listings.
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fatx_names.h"

// Paths here are game paths, far below this.
enum { PATH_BYTES = 1024 };

int __real_open(const char *path, int flags, ...);
int __real_stat(const char *path, struct stat *info);
int __real_mkdir(const char *path, mode_t mode);
int __real_unlink(const char *path);
int __real_access(const char *path, int mode);
int __real_rename(const char *old_path, const char *new_path);
DIR *__real_opendir(const char *path);
struct dirent *__real_readdir(DIR *dir);
int __real_closedir(DIR *dir);

// The folder part of path, with a separator at the end, or "" for none.
static void parent_of(const char *path, char *out, size_t size) {
    const char *slash = NULL;
    for (const char *p = path; *p; p++) { if (*p == '/' || *p == '\\') { slash = p; } }
    size_t length = slash ? (size_t)(slash - path) + 1 : 0;
    if (length >= size) { length = 0; }
    memcpy(out, path, length);
    out[length] = '\0';
}

// Finds short in folder's index and writes its long name; false if it isn't listed.
static bool lookup_long(const char *folder, const char *short_name, char *out, size_t size) {
    char index[PATH_BYTES];
    snprintf(index, sizeof(index), "%s%s", folder, FATX_NAMES_INDEX);
    FILE *file = fopen(index, "r");
    if (!file) { return false; }
    char line[600];
    bool found = false;
    const size_t short_length = strlen(short_name);
    while (!found && fgets(line, sizeof(line), file)) {
        if (strncmp(line, short_name, short_length) || line[short_length] != '\t') { continue; }
        char *name = line + short_length + 1;
        name[strcspn(name, "\r\n")] = '\0';
        if (strlen(name) < size) { strcpy(out, name); found = true; }
    }
    fclose(file);
    return found;
}

// Lists the long name of each long component this path creates, once, in that component's folder.
static void remember(const char *path) {
    char partial[PATH_BYTES];
    size_t o = 0;
    for (const char *p = path; *p;) {
        size_t length = strcspn(p, "/\\");
        if (length > FATX_NAME_MAX) {
            char folder[PATH_BYTES], short_name[FATX_NAME_MAX + 1], existing[PATH_BYTES];
            partial[o] = '\0';
            fatx_short_path(partial, folder, sizeof(folder));
            if (!folder[0] && partial[0]) { strcpy(folder, partial); }
            fatx_short_name(p, length, short_name);
            if (!lookup_long(folder, short_name, existing, sizeof(existing))) {
                char index[PATH_BYTES];
                snprintf(index, sizeof(index), "%s%s", folder, FATX_NAMES_INDEX);
                FILE *file = fopen(index, "a");
                if (file) { fprintf(file, "%s\t%.*s\n", short_name, (int)length, p); fclose(file); }
            }
        }
        if (o + length + 1 >= sizeof(partial)) { return; }
        memcpy(partial + o, p, length);
        o += length;
        p += length;
        if (*p) { partial[o++] = *p++; }
    }
}

#define SHORT_PATH(name, path) char name[PATH_BYTES]; const char *name##_use = fatx_short_path(path, name, sizeof(name)) ? name : path

int __wrap_open(const char *path, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) { va_list args; va_start(args, flags); mode = (mode_t)va_arg(args, int); va_end(args); }
    SHORT_PATH(stored, path);
    if (stored_use == stored && (flags & O_CREAT)) { remember(path); }
    return __real_open(stored_use, flags, mode);
}
int __wrap_stat(const char *path, struct stat *info) { SHORT_PATH(stored, path); return __real_stat(stored_use, info); }
int __wrap_mkdir(const char *path, mode_t mode) {
    SHORT_PATH(stored, path);
    if (stored_use == stored) { remember(path); }
    return __real_mkdir(stored_use, mode);
}
int __wrap_unlink(const char *path) { SHORT_PATH(stored, path); return __real_unlink(stored_use); }
int __wrap_access(const char *path, int mode) { SHORT_PATH(stored, path); return __real_access(stored_use, mode); }
int __wrap_rename(const char *old_path, const char *new_path) {
    SHORT_PATH(old_stored, old_path);
    SHORT_PATH(new_stored, new_path);
    if (new_stored_use == new_stored) { remember(new_path); }
    return __real_rename(old_stored_use, new_stored_use);
}

// Each open listing's folder, for its index. shortcut: 32 at once, which no game path nests near.
static struct { DIR *dir; char folder[PATH_BYTES]; } sListings[32];

DIR *__wrap_opendir(const char *path) {
    SHORT_PATH(stored, path);
    DIR *dir = __real_opendir(stored_use);
    if (!dir) { return NULL; }
    for (int i = 0; i < 32; i++) {
        if (sListings[i].dir) { continue; }
        sListings[i].dir = dir;
        const size_t length = strlen(stored_use);
        snprintf(sListings[i].folder, PATH_BYTES, "%s%s", stored_use,
            length && stored_use[length - 1] != '/' && stored_use[length - 1] != '\\' ? "/" : "");
        break;
    }
    return dir;
}

struct dirent *__wrap_readdir(DIR *dir) {
    for (;;) {
        struct dirent *entry = __real_readdir(dir);
        if (!entry || strcmp(entry->d_name, FATX_NAMES_INDEX) == 0) {
            if (!entry) { return NULL; }
            continue;
        }
        if (strlen(entry->d_name) == FATX_NAME_MAX && strchr(entry->d_name, '~')) {
            for (int i = 0; i < 32; i++) {
                if (sListings[i].dir != dir) { continue; }
                char long_name[sizeof(entry->d_name)];
                if (lookup_long(sListings[i].folder, entry->d_name, long_name, sizeof(long_name))) {
                    strcpy(entry->d_name, long_name);
                }
                break;
            }
        }
        return entry;
    }
}

int __wrap_closedir(DIR *dir) {
    for (int i = 0; i < 32; i++) { if (sListings[i].dir == dir) { sListings[i].dir = NULL; } }
    return __real_closedir(dir);
}
