#ifndef HAL_H
#define HAL_H
#include <stdint.h>

#define HAL_AUDIO_RATE 44100

void hal_draw_line(uint8_t y, const uint8_t *shades160); 
void hal_debug_putc(char c);                              // serial/UART de depuração

void hal_audio_push(const int16_t *stereo, unsigned frames);
#endif
