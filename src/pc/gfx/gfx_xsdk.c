#if defined(__XSDK__)
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include "xsdk/gpu.h"
#include "xsdk/present.h"
#include "xsdk/log.h"
#include "gfx_xsdk.h"
#include "gfx_xenos.h"
#include "platform/xsdk/keyboard_xsdk.h"
#include "platform/xsdk/input_usb.h"

static uint32_t width, height, native_width, native_height, frame_address;
static bool running, frame_pending, gpu_open, display_open, events_open;
static void close_window(void) {
    running = false;
    if (display_open) { xsdk_present_close(); display_open = false; }
    if (gpu_open) { (void)xsdk_gpu_close(); gpu_open = false; }
    if (events_open) { SDL_QuitSubSystem(SDL_INIT_EVENTS); events_open = false; }
    frame_pending = false;
}
static void fatal(const char *operation, NTSTATUS status) {
    fprintf(stderr, "xSDK %s failed: 0x%08X\n", operation, (unsigned)status);
    close_window();
    exit(EXIT_FAILURE);
}
static void init(const char *title) {
    (void)title;
    if (running) return;
    events_open = SDL_InitSubSystem(SDL_INIT_EVENTS);
    if (!events_open) { fprintf(stderr, "SDL3 events: %s\n", SDL_GetError()); exit(EXIT_FAILURE); }
    NTSTATUS status = xsdk_gpu_open();
    if (!NT_SUCCESS(status)) fatal("gpu_open", status);
    gpu_open = true;
    status = xsdk_present_open(XSDK_DISPLAY_720P, 2, 1, &width, &height);
    if (!NT_SUCCESS(status)) fatal("present_open", status);
    display_open = true;
    xsdk_log_write("render mode: Guide-compatible 720p %ux%u", width, height);
    native_width=width;native_height=height;
    width/=2;height/=2;
    xsdk_log_write("render scale: game %ux%u, presentation %ux%u",width,height,native_width,native_height);
    running = true;
}
static void keyboard(kb_callback_t down, kb_callback_t up, void (*all_up)(void),
                     void (*text)(char *), void (*editing)(char *, int)) {
    (void)down; (void)up; (void)all_up; (void)text; (void)editing;
}
static void scroll_callback(void (*callback)(float, float)) { (void)callback; }
static void events(void) {
    xsdk_keyboard_poll();
    xsdk_usb_poll();
    SDL_Event event;
    while (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) running = false;
}
static void main_loop(void (*iterate)(void)) { if (running) iterate(); }
static void dimensions(uint32_t *w, uint32_t *h) { *w = width; *h = height; }
static bool start_frame(void) {
    if (!running) return false;
    // rendering_init submits a clear before the first displayed game frame.
    // Multiple display lists may share one acquired buffer until swap_end.
    if (frame_pending) return true;
    NTSTATUS status = xsdk_present_begin(&frame_address, 5000);
    if (!NT_SUCCESS(status)) { fprintf(stderr, "xSDK present_begin: 0x%08X\n", (unsigned)status); running = false; return false; }
    frame_pending = true;
    return true;
}
// Rendering writes eDRAM; presentation resolves it to a paced front buffer.
static void swap_begin(void) {
    if (frame_pending) gfx_xenos_present(frame_address, native_width, native_height);
}
static void swap_end(void) {
    if (frame_pending) {
        xsdk_present_end(); frame_pending = false;
        static uint64_t last_report;
        uint64_t now = SDL_GetTicksNS();
        if (now - last_report >= 5000000000ULL) {
            XsdkPresentStats p; XsdkGpuStats g;
            xsdk_present_stats(&p); xsdk_gpu_stats(&g);
            xsdk_log_write("present: flips %u swaps %u landed %u system %u missed %u", p.flips, g.swaps, g.swap_fence_landed, p.system_frames, p.missed);
            last_report = now;
        }
    }
}
static double time_seconds(void) { return (double)SDL_GetTicksNS() / 1000000000.0; }
static void nothing(void) {}
static char *clipboard(void) { static char empty[] = ""; return empty; }
static void set_clipboard(const char *text) { (void)text; }
static void cursor(bool visible) { (void)visible; }
static void delay(unsigned ms) { SDL_Delay(ms); }
static int max_msaa(void) { return 0; }
static void title(const char *text) { (void)text; }
static bool focused(void) { return running; }
// Text/clipboard/window controls have no console equivalent in this first backend.
struct GfxWindowManagerAPI gfx_xsdk = {
    init, keyboard, scroll_callback, main_loop, dimensions, events, start_frame,
    swap_begin, swap_end, time_seconds, close_window, xsdk_usb_text_start, xsdk_usb_text_stop, clipboard,
    set_clipboard, cursor, delay, max_msaa, title, nothing, focused
};

#endif /* __XSDK__ */


