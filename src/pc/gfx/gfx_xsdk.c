#if defined(__XSDK__)
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include "xsdk/gpu.h"
#include "xsdk/present.h"
#include "xsdk/log.h"
#include "gfx_xsdk.h"
#include "gfx_xenos.h"
#include "gfx_xsdk_lut.h"
#include "pc/configfile.h"
#include "platform/xsdk/keyboard_xsdk.h"
#include "platform/xsdk/input_usb.h"

static uint32_t width, height, native_width, native_height, frame_address;
static uint64_t stat_wait_ns, stat_cpu_ns, frame_begin_ns;
static unsigned stat_frames;
static bool running, frame_pending, gpu_open, display_open, events_open;
// The display LUT, by register index (docs/HARDWARE.md in xSDK): each colour write moves the index on.
static volatile uint32_t *const display_registers = (volatile uint32_t *)0x7FC80000;
enum { LUT_RW_MODE = 0x1921, LUT_RW_INDEX = 0x1922, LUT_COLOR = 0x1925, LUT_WRITE_MASK = 0x1927 };
// xSDK leaves a linear LUT, with no adjustment for the screen: the player sets it in the display options.
void gfx_xsdk_apply_output_levels(void) {
    if (!display_open) return;
    display_registers[LUT_RW_MODE] = 0;
    display_registers[LUT_WRITE_MASK] = 7;
    display_registers[LUT_RW_INDEX] = 0;
    for (unsigned i = 0; i < 256; i++) {
        const uint32_t v = gfx_xsdk_lut_value(i, configXboxOutputGamma, configXboxLimitedRange);
        display_registers[LUT_COLOR] = (v << 20) | (v << 10) | v;
    }
}
// XAM loads its own LUT for the Guide: log it, since it shows the levels the console's own screens use, then put ours back.
static void restore_levels_after_guide(void) {
    static uint32_t last_system_frames, frame;
    static bool guide_seen;
    if (++frame % 30) return;
    XsdkPresentStats p;
    xsdk_present_stats(&p);
    if (p.system_frames != last_system_frames) { last_system_frames = p.system_frames; guide_seen = true; return; }
    if (!guide_seen) return;
    guide_seen = false;
    static const unsigned samples[5] = { 0, 16, 128, 235, 255 };
    uint32_t lut[5];
    display_registers[LUT_RW_MODE] = 0;
    for (unsigned i = 0; i < 5; i++) {
        display_registers[LUT_RW_INDEX] = samples[i];
        lut[i] = display_registers[LUT_COLOR];
    }
    xsdk_log_write("display: Guide LUT at 0,16,128,235,255: %08X %08X %08X %08X %08X", (unsigned)lut[0], (unsigned)lut[1], (unsigned)lut[2], (unsigned)lut[3], (unsigned)lut[4]);
    gfx_xsdk_apply_output_levels();
}
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
    // Three buffers: with two, a frame that misses one vblank waits for the next, locking to 30 FPS.
    status = xsdk_present_open(XSDK_DISPLAY_720P, 3, 1, &width, &height);
    if (!NT_SUCCESS(status)) fatal("present_open", status);
    display_open = true;
    gfx_xsdk_apply_output_levels();
    xsdk_log_write("render mode: Guide-compatible 720p %ux%u", width, height);
    native_width=width;native_height=height;
    // The game draws at this share of 720p and the present scales it up: resolves want widths in 32s, heights in 8s.
    static const unsigned render_percent[3] = { 100, 75, 50 };
    const unsigned percent = render_percent[configXboxRenderScale < 3 ? configXboxRenderScale : 0];
    width = (native_width * percent / 100) & ~31u; height = (native_height * percent / 100) & ~7u;
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
    const uint64_t wait_start = SDL_GetTicksNS();
    NTSTATUS status = xsdk_present_begin(&frame_address, 5000);
    frame_begin_ns = SDL_GetTicksNS();
    stat_wait_ns += frame_begin_ns - wait_start;
    if (!NT_SUCCESS(status)) { fprintf(stderr, "xSDK present_begin: 0x%08X\n", (unsigned)status); running = false; return false; }
    frame_pending = true;
    return true;
}
// Rendering writes eDRAM; presentation resolves it to a paced front buffer.
static void swap_begin(void) {
    if (frame_pending) {
        stat_cpu_ns += SDL_GetTicksNS() - frame_begin_ns; ++stat_frames;
        gfx_xenos_present(frame_address, native_width, native_height);
    }
}
static void swap_end(void) {
    if (frame_pending) {
        xsdk_present_end(); frame_pending = false;
        restore_levels_after_guide();
        // capture.txt beside the XEX asks for one frame in capture.raw, to compare colours with the PC.
        static unsigned capture_check;
        if (++capture_check % 60 == 0) {
            FILE *request = fopen("game:\\capture.txt", "r");
            if (request) { fclose(request); remove("game:\\capture.txt"); gfx_xenos_request_capture(); }
        }
        static uint64_t last_report;
        uint64_t now = SDL_GetTicksNS();
        if (now - last_report >= 5000000000ULL) {
            XsdkPresentStats p; XsdkGpuStats g;
            xsdk_present_stats(&p); xsdk_gpu_stats(&g);
            xsdk_log_write("present: flips %u swaps %u landed %u system %u missed %u frames %u cpu_ms %.2f wait_ms %.2f", p.flips, g.swaps, g.swap_fence_landed, p.system_frames, p.missed, stat_frames, stat_frames ? (double)stat_cpu_ns/1000000.0/stat_frames : 0.0, stat_frames ? (double)stat_wait_ns/1000000.0/stat_frames : 0.0);
            stat_frames = 0; stat_cpu_ns = stat_wait_ns = 0;
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


