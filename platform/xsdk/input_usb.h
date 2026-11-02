#ifndef COOPDX_INPUT_USB_H
#define COOPDX_INPUT_USB_H
#include "pc/controller/controller_api.h"
void xsdk_usb_poll(void);
void xsdk_usb_text_start(void);
void xsdk_usb_text_stop(void);
void xsdk_usb_mouse_window(void);
void xsdk_usb_mouse_relative(void);
extern struct ControllerAPI controller_usb_mouse;
#endif
