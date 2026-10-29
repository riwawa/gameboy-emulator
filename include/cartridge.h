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
    const uint8_t *rom;  
    bool owns_rom;        
    size_t rom_size;
    CartridgeHeader header;

    uint8_t *ram;
    size_t ram_size;

    bool mbc1;         
    bool ram_enable;   
    uint8_t bank1;     
    uint8_t bank2;     
    uint8_t mode;  
    uint32_t rom_banks; 
} Cartridge;

bool cartridge_init(Cartridge *cart, const uint8_t *rom, size_t size);   
bool cartridge_load(Cartridge *cart, const char *path);                  
uint8_t cartridge_read(const Cartridge *cart, uint16_t address);
void cartridge_free(Cartridge *cart);
void cartridge_write(Cartridge *cart, uint16_t address, uint8_t value);
uint8_t cartridge_ram_read(const Cartridge *cart, uint16_t address);            
void cartridge_ram_write(Cartridge *cart, uint16_t address, uint8_t value);

#endif