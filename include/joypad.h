#ifndef JOYPAD_H
#define JOYPAD_H
#include <stdint.h>
#include <stdbool.h>
#include "bus.h"

typedef enum {
    BTN_A, BTN_B, BTN_SELECT, BTN_START,
    BTN_RIGHT, BTN_LEFT, BTN_UP, BTN_DOWN
} joypad_button;

void joypad_init(Bus *bus);
uint8_t joypad_read(void);                       // FF00
void joypad_write(uint8_t value);                // FF00
void joypad_set(joypad_button b, bool pressed); 
#endif