#include "hal.h"
#include <stdio.h>
#include <string.h>
#include "hal_sdl.h"
static uint8_t quadro[144 * 160];
const uint8_t *hal_sdl_quadro(void) { return quadro; }

void hal_draw_line(uint8_t y, const uint8_t *s) { memcpy(&quadro[y * 160], s, 160); }
void hal_debug_putc(char c) { putchar(c); fflush(stdout); }