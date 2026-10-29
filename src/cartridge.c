#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/cartridge.h"

static void parse_header(Cartridge *cart){
    const uint8_t *rom=cart->rom;

    cart->header.type=rom[0x0147];
    cart->header.rom_size_code=rom[0x0148];
    cart->header.ram_size_code=rom[0x0149];

    size_t title_len=16;
    if(rom[0x0143]==0x80 || rom[0x0143]==0xC0){
        title_len=11;
    }

    memcpy(cart->header.title,&rom[0x0134], title_len);
    cart->header.title[title_len]='\0';
}

bool cartridge_init(Cartridge *cart, const uint8_t *rom, size_t size) {
    if (!cart || !rom) return false;
    if (size < 0x8000 || (size % 0x4000) != 0) return false;

    *cart = (Cartridge){0};
    cart->rom = rom;
    cart->rom_size = size;
    cart->rom_banks = (uint32_t)(size / 0x4000);
    parse_header(cart);

    uint8_t t = cart->header.type;
    if (t > 0x03) {
        fprintf(stderr, "Tipo de cartucho 0x%02X ainda nao suportado\n", t);
        *cart = (Cartridge){0};
        return false;
    }
    cart->mbc1 = (t != 0x00);
    cart->bank1 = 1;

    static const uint32_t ram_kb[6] = {0, 2, 8, 32, 128, 64};
    uint8_t rc = cart->header.ram_size_code;
    if (rc < 6 && (t == 0x02 || t == 0x03 || t == 0x00)) cart->ram_size = ram_kb[rc] * 1024u;
    if (cart->ram_size) {
        cart->ram = calloc(1, cart->ram_size);
        if (!cart->ram) { *cart = (Cartridge){0}; return false; }
    }
    return true;
}

bool cartridge_load(Cartridge *cart, const char *path) {
    if (!cart || !path) return false;

    FILE *file = fopen(path, "rb");
    if (!file) return false;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return false; }
    long file_size = ftell(file);
    if (file_size < 0 || fseek(file, 0, SEEK_SET) != 0) { fclose(file); return false; }

    uint8_t *rom = malloc((size_t)file_size);
    if (!rom) { fclose(file); return false; }
    size_t n = fread(rom, 1, (size_t)file_size, file);
    fclose(file);
    if (n != (size_t)file_size || !cartridge_init(cart, rom, (size_t)file_size)) {
        free(rom);
        return false;
    }
    cart->owns_rom = true;         
    return true;
}

uint8_t cartridge_read(const Cartridge *cart, uint16_t address) {
    if (!cart||!cart->rom) return 0xFF;

    if (address >= 0x8000) return 0xFF;
    if (!cart->mbc1) return cart->rom[address];           

    uint32_t bank;
    if (address < 0x4000)                               
        bank = cart->mode ? ((uint32_t)cart->bank2 << 5) : 0;
    else                                                  
        bank = ((uint32_t)cart->bank2 << 5) | cart->bank1;
    bank &= cart->rom_banks - 1;                        
    return cart->rom[(size_t)bank * 0x4000 + (address & 0x3FFF)];
}
void cartridge_free(Cartridge *cart) {
    if(!cart) return;
    if (cart->owns_rom) free((void *)cart->rom);
    free(cart->ram);
    *cart=(Cartridge){0};
}

void cartridge_write(Cartridge *cart, uint16_t address, uint8_t value) {
    if(!cart||!cart->rom||!cart->mbc1) return;
    switch (address >> 13) {                      
    case 0: cart->ram_enable = (value & 0x0F) == 0x0A; break;          // 0000-1FFF
    case 1: cart->bank1 = value & 0x1F;                                // 2000-3FFF
            if (cart->bank1 == 0) cart->bank1 = 1; break;
    case 2: cart->bank2 = value & 0x03; break;                         // 4000-5FFF
    case 3: cart->mode  = value & 0x01; break;                         // 6000-7FFF
    }
}

static size_t ram_index(const Cartridge *cart, uint16_t address) {
    size_t bank = cart->mode ? cart->bank2 : 0;
    return (bank * 0x2000 + (address - 0xA000)) % cart->ram_size;   // RAM de 2 KB espelha
}

uint8_t cartridge_ram_read(const Cartridge *cart, uint16_t address) {
    if (!cart || !cart->ram || !cart->ram_enable) return 0xFF;
    return cart->ram[ram_index(cart, address)];
}

void cartridge_ram_write(Cartridge *cart, uint16_t address, uint8_t value) {
    if (!cart || !cart->ram || !cart->ram_enable) return;
    cart->ram[ram_index(cart, address)] = value;
}