#include "lcd.h"
#include "ppu.h"

static lcd_context lcd;

lcd_context *lcd_get(void) { return &lcd; }

void lcd_init(void) {
    lcd = (lcd_context){0};
    lcd.lcdc = 0x91;  
    lcd.dma  = 0xFF;
    lcd.bgp  = 0xFC;
    lcd.obp0 = 0xFF;
    lcd.obp1 = 0xFF;
}

uint8_t lcd_read(uint16_t address) {
    uint8_t v = ((uint8_t *)&lcd)[address - 0xFF40];
    if (address == 0xFF41) v |= 0x80;   
    return v;
}

void lcd_write(uint16_t address, uint8_t value) {
    switch (address) {
    case 0xFF40: {
        bool was_on = lcd.lcdc & 0x80;
        lcd.lcdc = value;
        if (was_on && !(value & 0x80)) ppu_lcd_off();
        if (!was_on && (value & 0x80)) ppu_lcd_on();
        break;
    }
    case 0xFF41:   
        lcd.stat = (lcd.stat & 0x87) | (value & 0x78);
        ppu_stat_changed();
        break;
    case 0xFF44:  
        break;
    case 0xFF45:
        lcd.lyc = value;
        ppu_stat_changed();
        break;
    case 0xFF46: 
        lcd.dma = value; 
        ppu_oam_dma(value); 
        break;

    default:      
        ((uint8_t *)&lcd)[address - 0xFF40] = value;
        break;
    }
}