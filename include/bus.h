#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "cartridge.h"

typedef struct {
    Cartridge *cart;
    uint8_t wram[0x2000];
    uint8_t hram[0x7F];
} Bus;

void bus_init(Bus *bus, Cartridge *cart);

uint8_t bus_read(Bus *bus, uint16_t address);
void bus_write(Bus *bus, uint16_t address, uint8_t value);

uint16_t bus_read16(Bus *bus, uint16_t address);
void bus_write16(Bus *bus, uint16_t address, uint16_t value);

#endif
