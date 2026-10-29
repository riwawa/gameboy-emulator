#include "joypad.h"
#include <stdio.h>

static Bus *bus_ref;
static uint8_t select_bits = 0x30;   
static bool pressed[8];             
static uint8_t last_low = 0x0F;      

uint8_t joypad_read(void) {
    uint8_t v = 0xC0 | select_bits | 0x0F;      

    if (!(select_bits & 0x20)) {                 
        if (pressed[BTN_A])      v &= ~0x01;
        if (pressed[BTN_B])      v &= ~0x02;
        if (pressed[BTN_SELECT]) v &= ~0x04;
        if (pressed[BTN_START])  v &= ~0x08;
    }
    if (!(select_bits & 0x10)) {             
        if (pressed[BTN_RIGHT])  v &= ~0x01;
        if (pressed[BTN_LEFT])   v &= ~0x02;
        if (pressed[BTN_UP])     v &= ~0x04;
        if (pressed[BTN_DOWN])   v &= ~0x08;
    }
    return v;
}

static void update(void) {
    uint8_t now = joypad_read() & 0x0F;
    if (last_low & ~now) bus_ref->if_reg |= 0x10;   
    last_low = now;
}

void joypad_init(Bus *bus) {
    bus_ref = bus;
    select_bits = 0x30;
    for (int i = 0; i < 8; i++) pressed[i] = false;
    last_low = 0x0F;
}

void joypad_write(uint8_t value) {
    static int n = 0;
    if (n++ < 12) fprintf(stderr, "FF00 <- %02X\n", value);   // diagnostico
    select_bits = value & 0x30;
    update();
}

void joypad_set(joypad_button b, bool is_pressed) {
    pressed[b] = is_pressed;
    update();
}