#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char title[17];
    uint8_t type;
    uint8_t rom_size_code;
    uint8_t ram_size_code;
} CartridgeHeader;

typedef struct {
    uint8_t *rom;
    size_t rom_size;
    CartridgeHeader header;

} Cartridge;

bool cartridge_load(Cartridge *cart, const char *path);
uint8_t cartridge_read(const Cartridge *cart, uint16_t address);
void cartridge_free(Cartridge *cart);
void cartridge_write(Cartridge *cart, uint16_t address, uint8_t value);

#endif

