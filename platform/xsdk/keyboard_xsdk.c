#include <string.h>
#include <SDL3/SDL.h>
#include "xsdk/xam.h"
#include "xsdk/log.h"
#include "keyboard_xsdk.h"
static XAM_OVERLAPPED operation;
static uint16_t initial_text[512], result_text[512];
static bool pending, accept_result;
static void (*completed)(const char *, void *);
static void *completed_context;
static void finish(uint32_t status) {
    pending = false;
    if (status == 0 && operation.extended_error == 0 && accept_result && completed) {
        size_t length = 0;
        while (length < 511 && result_text[length]) ++length;
        result_text[length] = 0;
        char *utf8 = SDL_iconv_string("UTF-8", "UTF-16BE", (const char *)result_text, (length + 1) * 2);
        if (utf8) { completed(utf8, completed_context); SDL_free(utf8); }
    }
    accept_result = false;
    completed = NULL; completed_context = NULL;
    xsdk_log_write("keyboard: completed result %u extended %u", status, operation.extended_error);
}
bool xsdk_keyboard_open(const char *initial, void (*done)(const char *, void *), void *context) {
    if (pending) return false;
    uint32_t user;
    char tag[XSDK_XAM_GAMERTAG_BYTES];
    if (xsdk_xam_signed_in_user(&user, tag) != 0) user = 0;
    memset(initial_text, 0, sizeof initial_text);
    memset(result_text, 0, sizeof result_text);
    char *utf16 = SDL_iconv_string("UTF-16BE", "UTF-8", initial, strlen(initial) + 1);
    if (!utf16) return false;
    const uint16_t *source = (const uint16_t *)utf16;
    size_t count = 0;
    while (count < 511 && source[count]) { initial_text[count] = source[count]; ++count; }
    SDL_free(utf16);
    static const uint16_t title[] = {'S','M','6','4',' ','C','o','o','p','D','X',0};
    static const uint16_t description[] = {'E','d','i','t','a','r',' ','t','e','x','t','o',0};
    completed = done; completed_context = context; accept_result = true;
    uint32_t status = xsdk_xam_keyboard(user, title, description, initial_text, result_text, 512, &operation);
    pending = status == ERROR_IO_PENDING;
    xsdk_log_write("keyboard: opened result %u", status);
    if (!pending) finish(status);
    return status == 0 || status == ERROR_IO_PENDING;
}
void xsdk_keyboard_poll(void) {
    if (!pending) return;
    uint32_t status = xsdk_xam_wait(&operation, 0, NULL);
    if (status != ERROR_IO_INCOMPLETE && status != ERROR_IO_PENDING) finish(status);
}
void xsdk_keyboard_dismiss(void) { accept_result = false; }
bool xsdk_keyboard_busy(void) { return pending; }
