#include "joypad.h"

static Bus *bus_ref;
static uint8_t select_bits = 0x30;   // bits 4 e 5 escritos pelo jogo (0x30 = nada selecionado)
static bool pressed[8];              // estado real dos 8 botões
static uint8_t last_low = 0x0F;      // último valor dos bits 0..3 (para detectar a descida)

uint8_t joypad_read(void) {
    uint8_t v = 0xC0 | select_bits | 0x0F;        // bits 7-6 leem 1; botões soltos = 1

    if (!(select_bits & 0x20)) {                  // bit 5 = 0 -> botões de ação
        if (pressed[BTN_A])      v &= ~0x01;
        if (pressed[BTN_B])      v &= ~0x02;
        if (pressed[BTN_SELECT]) v &= ~0x04;
        if (pressed[BTN_START])  v &= ~0x08;
    }
    if (!(select_bits & 0x10)) {                  // bit 4 = 0 -> direcionais
        if (pressed[BTN_RIGHT])  v &= ~0x01;
        if (pressed[BTN_LEFT])   v &= ~0x02;
        if (pressed[BTN_UP])     v &= ~0x04;
        if (pressed[BTN_DOWN])   v &= ~0x08;
    }
    return v;
}

// Pede a interrupção de joypad se algum dos bits 0..3 foi de 1 para 0
static void update(void) {
    uint8_t now = joypad_read() & 0x0F;
    if (last_low & ~now) bus_ref->if_reg |= 0x10;   // IF bit 4
    last_low = now;
}

void joypad_init(Bus *bus) {
    bus_ref = bus;
    select_bits = 0x30;
    for (int i = 0; i < 8; i++) pressed[i] = false;
    last_low = 0x0F;
}

void joypad_write(uint8_t value) {
    select_bits = value & 0x30;    // só os bits 4 e 5 são graváveis
    update();
}

void joypad_set(joypad_button b, bool is_pressed) {
    pressed[b] = is_pressed;
    update();
}