#ifndef APU_H
#define APU_H

#include <stdint.h>
#include <stdbool.h>

void    apu_init(void);
void    apu_tick(void);                     
void    apu_div_tick(void);                 
void    apu_flush(void);                   
uint8_t apu_read(uint16_t address);           
void    apu_write(uint16_t address, uint8_t value);

#endif
