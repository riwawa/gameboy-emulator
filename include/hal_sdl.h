#ifndef HAL_SDL_H
#define HAL_SDL_H
#include <stdint.h>

// Último quadro completo recebido via hal_draw_line (160x144, tons 0..3)
const uint8_t *hal_sdl_quadro(void);
#endif