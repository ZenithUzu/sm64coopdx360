#if defined(__XSDK__)
#include <limits.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include "audio_sdl3.h"

// The engine produces native-endian, signed 16-bit stereo at 32 kHz.
// buffered() counts input frames, not bytes or converted 48 kHz device frames.
static SDL_AudioStream *stream;
static bool owns_audio;
static bool reported_error;

static void audio_shutdown(void) {
    if (stream) { SDL_DestroyAudioStream(stream); stream = NULL; }
    if (owns_audio) { SDL_QuitSubSystem(SDL_INIT_AUDIO); owns_audio = false; }
}
static bool audio_init(void) {
    if (stream) return true;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        fprintf(stderr, "SDL3 audio init: %s\n", SDL_GetError());
        return false;
    }
    owns_audio = true;
    const SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, 32000 };
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream || !SDL_ResumeAudioStreamDevice(stream)) {
        fprintf(stderr, "SDL3 audio open: %s\n", SDL_GetError());
        audio_shutdown();
        return false;
    }
    reported_error = false;
    return true;
}
static int audio_buffered(void) {
    if (!stream) return 0;
    const int bytes = SDL_GetAudioStreamQueued(stream);
    return bytes > 0 ? bytes / 4 : 0;
}
static int audio_desired(void) { return 1100; }
static void audio_play(const uint8_t *buf, size_t len) {
    if (!stream || !buf || len == 0 || len > INT_MAX || len % 4 != 0) return;
    if (audio_buffered() >= 6000) return;
    if (!SDL_PutAudioStreamData(stream, buf, (int)len) && !reported_error) {
        fprintf(stderr, "SDL3 audio queue: %s\n", SDL_GetError());
        reported_error = true;
    }
}
struct AudioAPI audio_sdl3 = {
    audio_init, audio_buffered, audio_desired, audio_play, audio_shutdown
};

#endif /* __XSDK__ */

