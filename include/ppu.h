#ifndef PPU_H
#define PPU_H
#include <stdint.h>
#include <stdbool.h>
#include "bus.h"
/*
    roda em paralelo com a CPU.
    conta pontos (dots). Cada ciclo-M da CPU equivale a 4 pontos
*/
void ppu_init(Bus *bus);
void ppu_tick(void);          // avança 1 ponto (dot)
void ppu_lcd_off(void);
void ppu_lcd_on(void);
void ppu_stat_changed(void);  
uint32_t ppu_frames(void);   
 
void ppu_oam_dma(uint8_t value);
#endif