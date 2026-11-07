#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fatx_names.h"

static const char *shorten(const char *name) {
    static char out[FATX_NAME_MAX + 1];
    fatx_short_name(name, strlen(name), out);
    return out;
}

int main(void) {
    assert(strcmp(shorten("mod.lua"), "mod.lua") == 0);
    const char *exact = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.tex";
    assert(strlen(exact) == 42 && strcmp(shorten(exact), exact) == 0);

    const char *a = "char-select-ec-foremanspike-meter-right.tex";
    const char *b = "char-select-ec-foremanspike-meter-left.tex";
    char sa[43], sb[43];
    strcpy(sa, shorten(a));
    strcpy(sb, shorten("char-select-ec-foremanspike-meter-right2.tex"));
    assert(strlen(sa) == 42 && strstr(sa, "~") && strcmp(sa + 38, ".tex") == 0);
    assert(strcmp(sa, sb) != 0);  // names sharing a start stay apart
    assert(strcmp(shorten(b), b) == 0);
    assert(strcmp(shorten(a), sa) == 0);  // the same name always maps the same way

    // A long name without an extension, and one whose dot is too far back to be one.
    assert(strlen(shorten("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz")) == 42);
    assert(strlen(shorten("abcdefghijklmnopqrstuvwxyz.abcdefghijklmnopqrstuvwxyz")) == 42);

    char path[256];
    assert(!fatx_short_path("game:/mods/star-road/actors/a.col", path, sizeof(path)));
    assert(fatx_short_path("game:/mods/star-road/actors/inside_castle_seg7_custom_collision_star_door.col", path, sizeof(path)));
    assert(strncmp(path, "game:/mods/star-road/actors/", 28) == 0 && strlen(path + 28) == 42);
    assert(fatx_short_path("a\\exclamation_box_outline_seg8_collision_08025F78.col\\b", path, sizeof(path)));
    assert(path[1] == '\\' && path[44] == '\\' && path[45] == 'b');
    assert(!fatx_short_path("game:/mods/star-road/actors/inside_castle_seg7_custom_collision_star_door.col", path, 20));

    printf("fatx_names_test: ok %s\n", sa);
    return 0;
}
