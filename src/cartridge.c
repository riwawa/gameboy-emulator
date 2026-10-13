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

bool cartridge_load(Cartridge *cart, const char *path) {
    if(!cart||!path) return false;

    FILE *file = fopen(path, "rb");
    if (!file) return false;

    if (fseek(file,0,SEEK_END)!=0){
        fclose(file);
        return false;
    }

    long file_size = ftell(file);
    if (file_size!=0x8000) {
        fclose(file);
        return false;
    }
    if (fseek(file,0,SEEK_SET)!=0){
        fclose(file);
        return false;
    }
    
    uint8_t *rom = malloc((size_t)file_size);
    if(!rom){
        fclose(file);
        return false;
    }

    size_t read_size = fread(rom, 1, (size_t)file_size, file);
    fclose(file);

    if (read_size!=(size_t)file_size){
        free(rom);
        return false;
    }

    if (rom[0x0147]!=0x00||rom[0x0148]!=0x00||rom[0x0149]!=0x00) {
        free(rom);
        return false;
    }

    free(cart->rom);
    cart->rom = rom;
    cart->rom_size = (size_t)file_size;
    parse_header(cart);
    return true;
}

uint8_t cartridge_read(const Cartridge *cart, uint16_t address) {
    if (!cart||!cart->rom) return 0xFF;

    if(address<0x8000){
        return cart->rom[address];
    }
    return 0xFF;
}
void cartridge_free(Cartridge *cart) {
    if(!cart) return;
    free(cart->rom);
    *cart=(Cartridge){0};
}

void cartridge_write(Cartridge *cart, uint16_t address, uint8_t value) {
    if(!cart||!cart->rom) return;
    (void)address;
    (void)value;
}
