#ifndef LCD_H
#define LCD_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t lcdc;  // FF40 controle
    uint8_t stat;  // FF41 status
    uint8_t scy;   // FF42 scroll Y
    uint8_t scx;   // FF43 scroll X
    uint8_t ly;    // FF44 linha atual (só leitura)
    uint8_t lyc;   // FF45 compara com LY
    uint8_t dma;   // FF46 dispara a cópia para a OAM
    uint8_t bgp;   // FF47 paleta do fundo
    uint8_t obp0;  // FF48 paleta de sprites 0
    uint8_t obp1;  // FF49 paleta de sprites 1
    uint8_t wy;    // FF4A janela Y
    uint8_t wx;    // FF4B janela X
} lcd_context;

typedef enum { MODE_HBLANK = 0, MODE_VBLANK = 1, MODE_OAM = 2, MODE_DRAW = 3 } ppu_mode;

// Bits do LCDC (FF40)
#define LCDC_LCD_ON(l)      ((l)->lcdc & 0x80)
#define LCDC_WIN_MAP(l)     ((l)->lcdc & 0x40)  // 0=9800, 1=9C00
#define LCDC_WIN_ON(l)      ((l)->lcdc & 0x20)
#define LCDC_TILE_DATA(l)   ((l)->lcdc & 0x10)  // 1=8000 (sem sinal), 0=8800 (com sinal)
#define LCDC_BG_MAP(l)      ((l)->lcdc & 0x08)  // 0=9800, 1=9C00
#define LCDC_OBJ_SIZE(l)    ((l)->lcdc & 0x04)  // 0=8x8, 1=8x16
#define LCDC_OBJ_ON(l)      ((l)->lcdc & 0x02)
#define LCDC_BG_ON(l)       ((l)->lcdc & 0x01)

lcd_context *lcd_get(void);
void lcd_init(void);
uint8_t lcd_read(uint16_t address);
void lcd_write(uint16_t address, uint8_t value);
#endif