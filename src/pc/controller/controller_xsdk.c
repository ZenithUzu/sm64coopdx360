#if defined(__XSDK__)
#include "controller_mouse.h"
#include "pc/gfx/gfx_xsdk.h"
#include <math.h>
#include "xsdk/xam.h"
#include "platform/xsdk/keyboard_xsdk.h"
#include <SDL3/SDL.h>
#include "controller_xsdk.h"
#include "pc/configfile.h"
extern bool djui_interactable_on_key_down(int key);
extern void djui_interactable_on_key_up(int key);
extern void djui_panel_pause_disconnect_key_update(int key);
static SDL_Gamepad *gamepad;
static bool initialized;
static unsigned slot;
static u32 last_buttons, raw_press = VK_INVALID;
static void buttons_changed(u32 buttons) {
    const u32 pressed=buttons & ~last_buttons, released=last_buttons & ~buttons;
    for (unsigned i=0; i<32; i++) {
        if (pressed & (1u<<i)) {
            raw_press=i;
            djui_panel_pause_disconnect_key_update(0x1000+i);
            djui_interactable_on_key_down(0x1000+i);
        }
        if (released & (1u<<i)) djui_interactable_on_key_up(0x1000+i);
    }
    last_buttons=buttons;
}
static void discover(void) {
    if (gamepad && (!SDL_GamepadConnected(gamepad) || slot != configGamepadNumber)) {
        buttons_changed(0); SDL_CloseGamepad(gamepad); gamepad=NULL;
    }
    if (gamepad) return;
    int count=0;
    SDL_JoystickID *ids=SDL_GetGamepads(&count);
    slot=configGamepadNumber;
    if (ids && slot<(unsigned)count) gamepad=SDL_OpenGamepad(ids[slot]);
    SDL_free(ids);
}
static void init(void) {
    if (initialized) return;
    initialized=SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    if (initialized) discover();
}
static bool bound(const unsigned *keys, u32 buttons) {
    for (unsigned i=0; i<MAX_BINDS; i++)
        if (keys[i]>=0x1000 && keys[i]<0x1020 && (buttons & (1u<<(keys[i]-0x1000)))) return true;
    return false;
}
static void stick(s8 *x, s8 *y, float ax, float ay, bool rotate, bool invert_x, bool invert_y) {
    if (rotate) { float old=ax; ax=-ay; ay=old; }
    if (invert_x) ax=-ax;
    if (invert_y) ay=-ay;
    float magnitude=sqrtf(ax*ax+ay*ay);
    float deadzone=fminf(configStickDeadzone*310.0f,32000.0f);
    if (magnitude<=deadzone) return;
    float dx=ax/magnitude, dy=ay/magnitude;
    float scale=1.0f/fmaxf(fabsf(dx),fabsf(dy));
    float maximum=32768.0f*scale;
    float output=fminf((magnitude-deadzone)*maximum/(maximum-deadzone)/256.0f,scale*127.0f);
    *x=(s8)(dx*output); *y=(s8)(-dy*output);
}
static void read_pad(OSContPad *pad) {
    if (!initialized) return;
    SDL_UpdateGamepads(); discover();
    if (xsdk_keyboard_busy() || xsdk_system_ui_open()) { buttons_changed(0); return; }
    if (!gamepad || configDisableGamepads) { buttons_changed(0); return; }
    u32 buttons=0;
    for (unsigned i=0; i<SDL_GAMEPAD_BUTTON_COUNT && i<26; i++)
        if (SDL_GetGamepadButton(gamepad,(SDL_GamepadButton)i)) buttons|=1u<<i;
    if (SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFT_TRIGGER)>7680) buttons|=1u<<26;
    if (SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)>7680) buttons|=1u<<27;
    buttons_changed(buttons);
    if (xsdk_keyboard_busy()) return;
    const struct { const unsigned *keys; u32 mask; } bindings[] = {
        {configKeyA,A_BUTTON},{configKeyB,B_BUTTON},{configKeyX,X_BUTTON},{configKeyY,Y_BUTTON},
        {configKeyStart,START_BUTTON},{configKeyL,L_TRIG},{configKeyR,R_TRIG},{configKeyZ,Z_TRIG},
        {configKeyCUp,U_CBUTTONS},{configKeyCDown,D_CBUTTONS},{configKeyCLeft,L_CBUTTONS},{configKeyCRight,R_CBUTTONS},
        {configKeyDUp,U_JPAD},{configKeyDDown,D_JPAD},{configKeyDLeft,L_JPAD},{configKeyDRight,R_JPAD}
    };
    for (unsigned i=0; i<sizeof bindings/sizeof bindings[0]; i++)
        if (bound(bindings[i].keys,buttons)) pad->button|=(u16)bindings[i].mask;
    stick(&pad->stick_x,&pad->stick_y,SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFTX),
        SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFTY),configStick.rotateLeft,configStick.invertLeftX,configStick.invertLeftY);
    stick(&pad->ext_stick_x,&pad->ext_stick_y,SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_RIGHTX),
        SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_RIGHTY),configStick.rotateRight,configStick.invertRightX,configStick.invertRightY);
    if (pad->ext_stick_x>64) pad->button|=R_CBUTTONS;
    if (pad->ext_stick_x<-64) pad->button|=L_CBUTTONS;
    if (pad->ext_stick_y>64) pad->button|=U_CBUTTONS;
    if (pad->ext_stick_y<-64) pad->button|=D_CBUTTONS;
    if (bound(configKeyStickUp,buttons)) pad->stick_y=127;
    if (bound(configKeyStickDown,buttons)) pad->stick_y=-128;
    if (bound(configKeyStickLeft,buttons)) pad->stick_x=-128;
    if (bound(configKeyStickRight,buttons)) pad->stick_x=127;
}
static u32 rawkey(void) { u32 key=raw_press; raw_press=VK_INVALID; return key; }
static void rumble(float strength,float seconds) {
    if (!gamepad || !isfinite(strength) || !isfinite(seconds)) return;
    strength=fmaxf(0,fminf(strength,1)); seconds=fmaxf(0,fminf(seconds,60));
    SDL_RumbleGamepad(gamepad,(Uint16)(strength*65535),(Uint16)(strength*65535),(Uint32)(seconds*1000));
}
static void stop(void) { if(gamepad) SDL_RumbleGamepad(gamepad,0,0,0); }
static void reconfigure(void) { if(initialized) discover(); }
static void shutdown(void) {
    stop(); buttons_changed(0);
    if(gamepad) SDL_CloseGamepad(gamepad);
    gamepad=NULL; raw_press=VK_INVALID;
    if(initialized) SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    initialized=false;
}
// Menus only (djui_cursor calls it with a panel open): the left stick moves the cursor and A clicks.
void controller_xsdk_menu_cursor(void) {
    static u64 last_ns;
    static float rest_x, rest_y;
    const u64 now_ns = SDL_GetTicksNS();
    float seconds = (float)(now_ns - last_ns) / 1000000000.0f;
    last_ns = now_ns;
    if (seconds > 0.1f) seconds = 0.1f;
    if (!gamepad || configDisableGamepads || xsdk_keyboard_busy() || xsdk_system_ui_open()) return;
    float axis[2] = { SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f,
                      SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f };
    for (unsigned i = 0; i < 2; i++) {
        const float size = axis[i] < 0 ? -axis[i] : axis[i];
        const float live = size > 0.25f ? (size - 0.25f) / 0.75f : 0;
        axis[i] = (axis[i] < 0 ? -1 : 1) * live * live * 1100.0f * seconds; // squared: slow near the center
    }
    rest_x += axis[0]; rest_y += axis[1];
    const int move_x = (int)rest_x, move_y = (int)rest_y;
    rest_x -= (float)move_x; rest_y -= (float)move_y;
    u32 width, height;
    gfx_xsdk.get_dimensions(&width, &height);
    mouse_window_x += move_x; mouse_window_y += move_y;
    if (mouse_window_x < 0) mouse_window_x = 0;
    if (mouse_window_y < 0) mouse_window_y = 0;
    if (mouse_window_x >= (int)width) mouse_window_x = (int)width - 1;
    if (mouse_window_y >= (int)height) mouse_window_y = (int)height - 1;
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH)) mouse_window_buttons |= L_MOUSE_BUTTON;
}

struct ControllerAPI controller_xsdk={0x1000,init,read_pad,rawkey,rumble,stop,reconfigure,shutdown};
#endif
