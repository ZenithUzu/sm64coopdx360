#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL.h>
#include "xsdk/kernel.h"
#include "xsdk/xam.h"
#include "xsdk/log.h"
#include "pc/controller/controller_keyboard.h"
#include "pc/controller/controller_bind_mapping.h"
#include "pc/controller/controller_mouse.h"
#include "pc/configfile.h"
#include "pc/gfx/gfx_xsdk.h"
#include "input_usb.h"
// XDK XINPUT_KEYSTROKE ABI; XAM ordinal 0x193 (Xenia xam_table.inc).
typedef struct { uint16_t key, unicode, flags; uint8_t user, hid; } UsbKey;
_Static_assert(sizeof(UsbKey)==8, "XINPUT_KEYSTROKE ABI");
typedef uint32_t (*ReadKeys)(uint32_t, uint32_t, UsbKey *);
typedef uint32_t (*ReadMouse)(uint8_t *, int16_t *, int16_t *, int16_t *);
static ReadKeys read_keys;
static ReadMouse read_mouse;
static bool initialized, text_active, was_system_ui;
static uint32_t held_mouse, previous_window_mouse, raw_mouse=VK_INVALID;
static int delta_x, delta_y;
static float wheel;
static void resolve(const char *name, unsigned ordinal, void *function) {
    HANDLE module=0; void *address=NULL;
    if (XexGetModuleHandle(name,&module)==0) (void)XexGetProcedureAddress(module,ordinal,&address);
    *(void **)function=address;
}
static void init(void) {
    if(initialized) return;
    initialized=true;
    controller_bind_init();
    resolve("xam.xex",0x193,&read_keys);
    resolve("xbdm.xex",0x90,&read_mouse);
    uint32_t w,h; gfx_xsdk.get_dimensions(&w,&h);
    mouse_window_x=w/2; mouse_window_y=h/2;
    xsdk_log_write("USB input: keyboard API %s; mouse API %s",read_keys?"available":"missing",read_mouse?"available":"missing");
}
void xsdk_usb_text_start(void) { text_active=true; }
void xsdk_usb_text_stop(void) { text_active=false; }
static void text_character(uint16_t c) {
    if(c<32 || c==127 || (c>=0xd800 && c<=0xdfff)) return;
    char text[4]={0};
    if(c<0x80) text[0]=(char)c;
    else if(c<0x800) {text[0]=(char)(0xc0|(c>>6));text[1]=(char)(0x80|(c&63));}
    else {text[0]=(char)(0xe0|(c>>12));text[1]=(char)(0x80|((c>>6)&63));text[2]=(char)(0x80|(c&63));}
    keyboard_on_text_input(text);
}
void xsdk_usb_poll(void) {
    init();
    bool system=xsdk_system_ui_open();
    if(system && !was_system_ui) keyboard_on_all_keys_up();
    was_system_ui=system;
    if(read_keys) for(unsigned n=0;n<64;++n) {
        UsbKey k={0};
        if(read_keys(0xff,0x40000002u,&k)!=0) break;
        if(system) continue;
        static bool logged_key;
        if(!logged_key) { xsdk_log_write("USB keyboard: received HID %u VK %u",k.hid,k.key); logged_key=true; }
        int scan=translate_sdl_scancode(k.hid);
        // HID usages are SDL USB scancodes; fallback for extended modifiers.
        if(!scan) {
            switch(k.key) {
            case 0xa0:scan=SCANCODE_SHIFT_LEFT;break; case 0xa1:scan=SCANCODE_SHIFT_RIGHT;break;
            case 0xa2:scan=SCANCODE_CONTROL_LEFT;break;case 0xa3:scan=SCANCODE_CONTROL_RIGHT;break;
            case 0xa4:scan=SCANCODE_ALT_LEFT;break;case 0xa5:scan=SCANCODE_ALT_RIGHT;break;
            default:break;
            }
        }
        if(k.flags&2) {if(scan) keyboard_on_key_up(scan);}
        else if(k.flags&(1|4)) {if(scan) keyboard_on_key_down(scan);if(text_active && (!(k.flags&0x30) || (k.flags&0x40))) text_character(k.unicode);}
    }
    if(read_mouse) {
        uint8_t buttons=0;int16_t x=0,y=0,z=0;
        uint32_t status=read_mouse(&buttons,&x,&y,&z);
        if((int32_t)status>=0 && !system) {
            mouse_init_ok=true;
            static bool logged_mouse;
            if(!logged_mouse && (buttons||x||y||z)) { xsdk_log_write("USB mouse: movement/buttons received"); logged_mouse=true; }
            uint32_t next=(buttons&1)|((buttons&2)<<1)|((buttons&4)>>1)|(buttons&24);
            uint32_t pressed=next&~held_mouse;
            for(unsigned i=0;i<5;++i) if(pressed&(1u<<i)) raw_mouse=0x1101+i;
            held_mouse=next;delta_x+=x;delta_y+=y;wheel+=z/120.0f;
            uint32_t w,h;gfx_xsdk.get_dimensions(&w,&h);
            mouse_window_x+=x;mouse_window_y+=y;
            if(mouse_window_x<0)mouse_window_x=0;
            if(mouse_window_y<0)mouse_window_y=0;
            if(mouse_window_x>=(int)w)mouse_window_x=w-1;
            if(mouse_window_y>=(int)h)mouse_window_y=h-1;
        } else held_mouse=0;
    }
}
void xsdk_usb_mouse_window(void) {
    mouse_window_buttons=held_mouse;
    mouse_window_buttons_pressed=held_mouse&~previous_window_mouse;
    mouse_window_buttons_released=previous_window_mouse&~held_mouse;
    previous_window_mouse=held_mouse;
}
void xsdk_usb_mouse_relative(void) {
    mouse_x=mouse_relative_enabled?delta_x:0;mouse_y=mouse_relative_enabled?delta_y:0;
    delta_x=delta_y=0;mouse_buttons=held_mouse;
    mouse_scroll_x=0;mouse_scroll_y=wheel;
    if(wheel>0)mouse_buttons|=MWHEELUP;else if(wheel<0)mouse_buttons|=MWHEELDOWN;
    if(wheel)mouse_scroll_timestamp=SDL_GetTicks();
    wheel=0;
}
static bool bound(const unsigned *keys) {
    for(unsigned i=0;i<MAX_BINDS;++i)if(keys[i]>=0x1101 && keys[i]<=0x1107 && (mouse_buttons&(1u<<(keys[i]-0x1101))))return true;
    return false;
}
extern bool gInteractableOverridePad;
static void read_pad(OSContPad *pad) {
    if(gInteractableOverridePad)controller_mouse_leave_relative();else controller_mouse_enter_relative();
    xsdk_usb_mouse_relative();
    if(gInteractableOverridePad || was_system_ui)return;
    const struct {const unsigned *keys;unsigned mask;} bindings[]={
        {configKeyA,A_BUTTON},{configKeyB,B_BUTTON},{configKeyX,X_BUTTON},{configKeyY,Y_BUTTON},
        {configKeyStart,START_BUTTON},{configKeyZ,Z_TRIG},{configKeyL,L_TRIG},{configKeyR,R_TRIG},
        {configKeyCUp,U_CBUTTONS},{configKeyCDown,D_CBUTTONS},{configKeyCLeft,L_CBUTTONS},{configKeyCRight,R_CBUTTONS}};
    for(unsigned i=0;i<sizeof bindings/sizeof bindings[0];++i)if(bound(bindings[i].keys))pad->button|=bindings[i].mask;
}
static uint32_t rawkey(void){uint32_t k=raw_mouse;raw_mouse=VK_INVALID;return k;}
static void shutdown(void){keyboard_on_all_keys_up();held_mouse=0;delta_x=delta_y=0;}
struct ControllerAPI controller_usb_mouse={0x1100,init,read_pad,rawkey,NULL,NULL,NULL,shutdown};
