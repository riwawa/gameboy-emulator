#include "audio_sdl.h"
#include "hal.h"
#include <SDL.h>
#include <stdio.h>

static SDL_AudioDeviceID dev;
static bool started;

#define FRAME_SAMPLES   ((Uint32)((HAL_AUDIO_RATE * 70224ULL) / 4194304ULL + 1))   
#define BYTES_PER_FRAME 4                                           
#define START_LEVEL     (2 * FRAME_SAMPLES * BYTES_PER_FRAME)    
#define MAX_LEVEL       (6 * FRAME_SAMPLES * BYTES_PER_FRAME)     

bool audio_init(void) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL audio: %s\n", SDL_GetError());
        return false;
    }
    SDL_AudioSpec want = {0}, have;
    want.freq = HAL_AUDIO_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = NULL;                      
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!dev) {
        fprintf(stderr, "SDL_OpenAudioDevice: %s\n", SDL_GetError());
        return false;
    }
    if (have.freq != HAL_AUDIO_RATE || have.channels != 2 || have.format != AUDIO_S16SYS) {
        fprintf(stderr, "Audio: formato diferente do pedido (%d Hz)\n", have.freq);
        SDL_CloseAudioDevice(dev);
        dev = 0;
        return false;
    }
    fprintf(stderr, "Audio ok: driver=%s, %d Hz, %d canais\n",
        SDL_GetCurrentAudioDriver(), have.freq, have.channels);
    return true;                           
}

void audio_quit(void) {
    if (dev) SDL_CloseAudioDevice(dev);
    dev = 0;
}

void hal_audio_push(const int16_t *stereo, unsigned frames) {
    if (!dev) return;      
    static int n = 0;
    if (n++ < 3) fprintf(stderr, "push: %u amostras, fila=%u\n", frames, SDL_GetQueuedAudioSize(dev));                   
    Uint32 queued = SDL_GetQueuedAudioSize(dev);
    if (queued > MAX_LEVEL) return;
    SDL_QueueAudio(dev, stereo, frames * BYTES_PER_FRAME);
    if (!started && queued + frames * BYTES_PER_FRAME >= START_LEVEL) {
        SDL_PauseAudioDevice(dev, 0);
        started = true;
    }
}
