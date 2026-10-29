#include "ui.h"
#include <SDL.h>
#include <stdio.h>
#include "hal_sdl.h"
#include "joypad.h"

#define XRES  160
#define YRES  144
#define SCALE 4

static SDL_Window   *win;
static SDL_Renderer *ren;
static SDL_Texture  *tex;
static uint32_t pixels[XRES * YRES];

static const uint32_t palette[4] = {0xFFE0F8D0, 0xFF88C070, 0xFF346856, 0xFF081820};

bool ui_init(const char *title) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");   // pixels nítidos, sem suavizar

    win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           XRES * SCALE, YRES * SCALE, SDL_WINDOW_RESIZABLE);
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return false; }

    ren = SDL_CreateRenderer(win, -1, 0);
    if (!ren) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); return false; }
    SDL_RenderSetLogicalSize(ren, XRES, YRES);          // escala mantendo a proporção

    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                            SDL_TEXTUREACCESS_STREAMING, XRES, YRES);
    if (!tex) { fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError()); return false; }
    return true;
}

void ui_present(void) {
    const uint8_t *shades = hal_sdl_quadro();
    for (int i = 0; i < XRES * YRES; i++) pixels[i] = palette[shades[i] & 3];
    SDL_UpdateTexture(tex, NULL, pixels, XRES * (int)sizeof(uint32_t));
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
}

bool ui_poll(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) return false;

        if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) && !e.key.repeat) {
            bool down = e.type == SDL_KEYDOWN;
            switch (e.key.keysym.sym) {
            case SDLK_x:         joypad_set(BTN_A, down);      break;
            case SDLK_z:         joypad_set(BTN_B, down);      break;
            case SDLK_RETURN:    joypad_set(BTN_START, down);  break;
            case SDLK_BACKSPACE: joypad_set(BTN_SELECT, down); break;
            case SDLK_RIGHT:     joypad_set(BTN_RIGHT, down);  break;
            case SDLK_LEFT:      joypad_set(BTN_LEFT, down);   break;
            case SDLK_UP:        joypad_set(BTN_UP, down);     break;
            case SDLK_DOWN:      joypad_set(BTN_DOWN, down);   break;
            case SDLK_F12: if (down) ui_screenshot("tela.bmp"); break;
            default: break;
            }
        }
    }
    return true;
}

// 70224 pontos por quadro a 4.194.304 Hz = ~16,74 ms por quadro
void ui_wait_frame(void) {
    static uint64_t next = 0;
    const uint64_t freq = SDL_GetPerformanceFrequency();
    const uint64_t frame_ticks = (uint64_t)((double)freq * 70224.0 / 4194304.0);
    uint64_t now = SDL_GetPerformanceCounter();

    if (next == 0 || now > next + 5 * frame_ticks) next = now;   // evita "correr atrás"
    next += frame_ticks;

    if (now < next) {
        uint64_t ms = (next - now) * 1000 / freq;
        if (ms > 2) SDL_Delay((uint32_t)(ms - 1));               // dorme a maior parte
        while (SDL_GetPerformanceCounter() < next) { }           // espera o resto ativamente
    }
}

bool ui_screenshot(const char *path) {
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, XRES, YRES, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!s) return false;
    SDL_memcpy(s->pixels, pixels, sizeof(pixels));
    bool ok = SDL_SaveBMP(s, path) == 0;
    SDL_FreeSurface(s);
    return ok;
}

void ui_quit(void) {
    if (tex) SDL_DestroyTexture(tex);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    SDL_Quit();
}