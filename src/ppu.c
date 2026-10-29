#include "ppu.h"
#include "lcd.h"
#include "hal.h"

#define XRES          160
#define YRES          144
#define DOTS_PER_LINE 456
#define MODE2_DOTS    80
#define MODE3_DOTS    172   // simplificação: na realidade varia de 172 a 289
#define LINES         154

static struct {
    Bus *bus;
    uint32_t line_dots;   // pontos dentro da linha atual (0..455)
    uint32_t frames;
    bool stat_line;       // nível anterior da linha de interrupção STAT
} ppu;

// Estado da janela (reinicia a cada quadro)
static bool win_y_cond;     // vira true quando LY == WY e fica true até o VBlank
static uint8_t win_line;    // contador interno de linha da janela

static void request_interrupt(int bit) { ppu.bus->if_reg |= (1u << bit); }

static ppu_mode get_mode(void) { return (ppu_mode)(lcd_get()->stat & 3); }

// A interrupção STAT dispara na BORDA DE SUBIDA do OU de todas as condições
// habilitadas ("STAT blocking").
void ppu_stat_changed(void) {
    lcd_context *l = lcd_get();
    l->stat = (l->stat & ~0x04) | (l->ly == l->lyc ? 0x04 : 0);   // flag LY==LYC

    bool line = false;
    if (l->stat & 0x40 && (l->stat & 0x04))   line = true;   // LY == LYC
    if (l->stat & 0x20 && get_mode() == MODE_OAM)    line = true;
    if (l->stat & 0x10 && get_mode() == MODE_VBLANK) line = true;
    if (l->stat & 0x08 && get_mode() == MODE_HBLANK) line = true;

    if (line && !ppu.stat_line) request_interrupt(1);
    ppu.stat_line = line;
}

static void set_mode(ppu_mode m) {
    lcd_context *l = lcd_get();
    l->stat = (l->stat & ~3) | m;
    ppu_stat_changed();
}

void ppu_init(Bus *bus) {
    ppu.bus = bus;
    ppu.line_dots = 0;
    ppu.frames = 0;
    ppu.stat_line = false;
    win_y_cond = false;
    win_line = 0;
    lcd_init();
    set_mode(MODE_OAM);
}

void ppu_lcd_off(void) {
    lcd_context *l = lcd_get();
    win_y_cond = false;
    win_line = 0;
    l->ly = 0;
    ppu.line_dots = 0;
    l->stat &= ~3;          // modo 0
    ppu.stat_line = false;
}

void ppu_lcd_on(void) {
    win_y_cond = false;
    win_line = 0;
    lcd_get()->ly = 0;
    ppu.line_dots = 0;
    set_mode(MODE_OAM);
}

uint32_t ppu_frames(void) { return ppu.frames; }

// Cor (0..3) de um pixel de um mapa de tiles (fundo e janela usam o mesmo código)
static uint8_t tile_pixel(uint16_t map_base, bool unsigned_ids, uint8_t map_x, uint8_t map_y) {
    const uint8_t *vram = ppu.bus->vram;
    uint8_t id = vram[map_base + (map_y >> 3) * 32 + (map_x >> 3)];
    uint16_t tile = unsigned_ids ? (uint16_t)id * 16 : (uint16_t)(0x1000 + (int8_t)id * 16);
    uint8_t row = map_y & 7;
    uint8_t lo = vram[tile + row * 2];
    uint8_t hi = vram[tile + row * 2 + 1];
    int bit = 7 - (map_x & 7);
    return (uint8_t)((((hi >> bit) & 1) << 1) | ((lo >> bit) & 1));
}

static void render_line(void) {
    lcd_context *l = lcd_get();
    Bus *b = ppu.bus;
    uint8_t y = l->ly;
    uint8_t out[XRES];                     // a linha pronta; quem guarda o quadro é a plataforma
    uint8_t bg_idx[XRES] = {0};            // índice de cor (0..3) do fundo/janela

    if (y == l->wy) win_y_cond = true;

    // ---- Fundo e janela ----
    if (l->lcdc & 0x01) {
        bool unsigned_ids = (l->lcdc & 0x10) != 0;
        uint16_t bg_map = (l->lcdc & 0x08) ? 0x1C00 : 0x1800;
        uint8_t map_y = (uint8_t)(y + l->scy);
        for (int x = 0; x < XRES; x++)
            bg_idx[x] = tile_pixel(bg_map, unsigned_ids, (uint8_t)(x + l->scx), map_y);

        int wx = (int)l->wx - 7;           // WX guarda a posição + 7
        if ((l->lcdc & 0x20) && win_y_cond && wx < XRES) {
            uint16_t win_map = (l->lcdc & 0x40) ? 0x1C00 : 0x1800;
            for (int x = (wx < 0 ? 0 : wx); x < XRES; x++)
                bg_idx[x] = tile_pixel(win_map, unsigned_ids, (uint8_t)(x - wx), win_line);
            win_line++;                    // só avança nas linhas em que a janela apareceu
        }
    }

    // ---- Sprites ----
    uint8_t obj_color[XRES] = {0};
    uint8_t obj_attr[XRES] = {0};
    if (l->lcdc & 0x02) {
        int h = (l->lcdc & 0x04) ? 16 : 8;

        // 1) Seleção: os 10 primeiros sprites da OAM que cruzam esta linha
        int sel[10], n = 0;
        for (int i = 0; i < 40 && n < 10; i++) {
            int sy = b->oam[i * 4] - 16;
            if (y >= sy && y < sy + h) sel[n++] = i;
        }
        // 2) Prioridade (DMG): menor X vence; empate: menor índice na OAM
        for (int a = 1; a < n; a++) {
            int k = sel[a], j = a - 1;
            while (j >= 0 && b->oam[sel[j] * 4 + 1] > b->oam[k * 4 + 1]) { sel[j + 1] = sel[j]; j--; }
            sel[j + 1] = k;
        }
        // 3) Desenha do mais prioritário ao menos; o primeiro pixel opaco fica
        for (int s = 0; s < n; s++) {
            const uint8_t *o = &b->oam[sel[s] * 4];
            int sx = o[1] - 8;
            uint8_t tile = o[2], attr = o[3];
            int r = y - (o[0] - 16);                 // linha dentro do sprite
            if (attr & 0x40) r = h - 1 - r;          // flip vertical
            if (h == 16) tile = (uint8_t)((tile & 0xFE) + (r >> 3));
            uint16_t addr = (uint16_t)tile * 16 + (r & 7) * 2;
            uint8_t lo = b->vram[addr], hi = b->vram[addr + 1];

            for (int px = 0; px < 8; px++) {
                int x = sx + px;
                if (x < 0 || x >= XRES || obj_color[x]) continue;
                int bit = (attr & 0x20) ? px : 7 - px;   // flip horizontal
                uint8_t c = (uint8_t)((((hi >> bit) & 1) << 1) | ((lo >> bit) & 1));
                if (c == 0) continue;                    // cor 0 é transparente
                obj_color[x] = c;
                obj_attr[x] = attr;
            }
        }
    }

    // ---- Composição final ----
    for (int x = 0; x < XRES; x++) {
        bool obj_wins = obj_color[x] &&
                        !((obj_attr[x] & 0x80) && bg_idx[x] != 0);   // "BG sobre OBJ"
        if (obj_wins) {
            uint8_t pal = (obj_attr[x] & 0x10) ? l->obp1 : l->obp0;
            out[x] = (pal >> (obj_color[x] * 2)) & 3;
        } else {
            out[x] = (l->bgp >> (bg_idx[x] * 2)) & 3;
        }
    }
    hal_draw_line(y, out);        // entrega a linha pronta à plataforma
}

// DMA da OAM: copia 160 bytes de (value << 8) para a OAM.
// Simplificação: copia tudo na hora, sem os 160 ciclos-M nem o bloqueio do bus.
void ppu_oam_dma(uint8_t value) {
    for (int i = 0; i < 0xA0; i++)
        ppu.bus->oam[i] = bus_read(ppu.bus, (uint16_t)((value << 8) | i));
}

void ppu_tick(void) {
    lcd_context *l = lcd_get();
    if (!(l->lcdc & 0x80)) return;     // LCD desligado: a PPU para

    ppu.line_dots++;

    if (l->ly < YRES) {
        if (ppu.line_dots == MODE2_DOTS)
            set_mode(MODE_DRAW);
        else if (ppu.line_dots == MODE2_DOTS + MODE3_DOTS) {
            render_line();
            set_mode(MODE_HBLANK);
        }
    }

    if (ppu.line_dots == DOTS_PER_LINE) {
        ppu.line_dots = 0;
        l->ly++;

        if (l->ly == YRES) {
            win_y_cond = false;
            win_line = 0;
            set_mode(MODE_VBLANK);
            request_interrupt(0);
            ppu.frames++;
        } else if (l->ly >= LINES) {   // fim do quadro
            l->ly = 0;
            set_mode(MODE_OAM);
        } else if (l->ly < YRES) {
            set_mode(MODE_OAM);
        }
        ppu_stat_changed();
    }
}