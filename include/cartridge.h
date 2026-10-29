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

typedef enum { MBC_NONE, MBC_1, MBC_2, MBC_3, MBC_5 } MbcKind;

typedef struct {
    uint8_t  s, m, h;     
    uint16_t d;     
    bool     halt;   
    bool     carry;        
    uint32_t sub;      
    uint8_t  latched[5]; 
    uint8_t  latch_prev;    
} Rtc;

typedef struct {
    const uint8_t *rom;   
    bool owns_rom;        
    size_t rom_size;
    uint32_t rom_banks;  
    CartridgeHeader header;

    uint8_t *ram;
    size_t ram_size;
    bool has_battery;  

    uint8_t *save;
    size_t   save_size;  
    bool     save_dirty; 

    MbcKind mbc;
    bool ram_enable;      

    // MBC1
    uint8_t bank1;      
    uint8_t bank2;   
    uint8_t mode;       

    // MBC3 / MBC5
    uint16_t rom_bank; 
    uint8_t  ram_sel;   
    bool     has_rtc;
    Rtc      rtc;
} Cartridge;

bool cartridge_init(Cartridge *cart, const uint8_t *rom, size_t size);   
bool cartridge_load(Cartridge *cart, const char *path);                
uint8_t cartridge_read(const Cartridge *cart, uint16_t address);
void cartridge_free(Cartridge *cart);
void cartridge_write(Cartridge *cart, uint16_t address, uint8_t value);
uint8_t cartridge_ram_read(const Cartridge *cart, uint16_t address);            
void cartridge_ram_write(Cartridge *cart, uint16_t address, uint8_t value);
void cartridge_tick(Cartridge *cart);
bool cartridge_save_import(Cartridge *cart, size_t got, uint64_t now); 
size_t cartridge_save_export(Cartridge *cart, uint64_t now); 

#endif
