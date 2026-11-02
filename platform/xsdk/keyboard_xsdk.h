#ifndef COOPDX_KEYBOARD_XSDK_H
#define COOPDX_KEYBOARD_XSDK_H
#include <stdbool.h>
bool xsdk_keyboard_open(const char *initial, void (*done)(const char *, void *), void *context);
void xsdk_keyboard_poll(void);
void xsdk_keyboard_dismiss(void);
bool xsdk_keyboard_busy(void);
#endif
