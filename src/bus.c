#include "../include/bus.h"
#include "cartridge.h"

#include <string.h>
/*
Memory Map:
https://gbdev.io/pandocs/Memory_Map.html#memory-map

0x0000 - 0x00FF : Boot ROM (disabled after boot)
0x0100 - 0x3FFF : ROM Bank 0 (fixed, from cartridge)
0x4000 - 0x7FFF : ROM Bank N (switchable via MBC)

0x8000 - 0x9FFF : Video RAM (VRAM) - 8 KB
0xA000 - 0xBFFF : External RAM (on cartridge, if present)
0xC000 - 0xCFFF : Work RAM Bank 0 (WRAM) - 4 KB
0xD000 - 0xDFFF : Work RAM Bank 1 (WRAM) - 4 KB (in CGB mode, switchange bank 1-7)
0xE000 - 0xFDFF : Echo RAM (mirror of 0xC000-0xDDFF)
0xFE00 - 0xFE9F : Object Attribute Memory (OAM) - Sprite data
0xFEA0 - 0xFEFF : Unusable (reads as 0x00)

0xFF00 - 0xFF7F : I/O Registers (hardware control)
0xFF80 - 0xFFFE : High RAM (HRAM) - 127 bytes
0xFFFF          : Interrupt Enable Register (IE)
*/

void bus_init(Bus *bus, Cartridge *cart) {
    bus->cart = cart;
    memset(bus->wram, 0x00, sizeof(bus->wram));
    memset(bus->hram, 0x00, sizeof(bus->hram));
}
uint8_t bus_read(Bus *bus, uint16_t address) {
    if (!bus) return 0xFF;

    if (address < 0x8000) {
        return cartridge_read(bus->cart, address);
    }
    if (address >= 0xC000 && address <= 0xDFFF) {
        return bus->wram[address - 0xC000];
    }
    if (address >= 0xFF80 && address <= 0xFFFE) {
        return bus->hram[address - 0xFF80];
    }
    return 0xFF;
}
void bus_write(Bus *bus, uint16_t address, uint8_t value){
    if (!bus) return; 
    if (address<0x8000){
        //ROM DATA
        cartridge_write(bus->cart, address, value);
        return;
    }
    if (address >= 0xC000 && address <= 0xDFFF) {
        bus->wram[address - 0xC000] = value;
        return;
    }

    if (address >= 0xFF80 && address <= 0xFFFE) {
        bus->hram[address - 0xFF80] = value;
        return;
    }
}
uint16_t bus_read16(Bus *bus, uint16_t address) {
    uint16_t lo = bus_read(bus, address);
    uint16_t hi = bus_read(bus, (uint16_t)(address + 1));

    return lo | (hi << 8);
}
void bus_write16(Bus *bus, uint16_t address, uint16_t value) {
    bus_write(bus, address, value & 0xFF);
    bus_write(bus, (uint16_t)(address + 1), (value >> 8) & 0xFF);
}