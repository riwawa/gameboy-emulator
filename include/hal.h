#ifndef HAL_H
#define HAL_H
#include <stdint.h>

void hal_draw_line(uint8_t y, const uint8_t *shades160);  
void hal_debug_putc(char c);                            
#endif