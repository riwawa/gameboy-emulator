#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "bus.h"

void    timer_init(Bus *bus);
void    timer_tick(void);                       // chamar 1x por ciclo-M
uint8_t timer_read(uint16_t address);           // FF04..FF07
void    timer_write(uint16_t address, uint8_t value);