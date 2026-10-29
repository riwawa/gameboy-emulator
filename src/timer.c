#include "timer.h"

static struct {
    Bus     *bus;
    uint16_t counter;
    uint8_t  tima, tma, tac;
    uint8_t  state;         
} t;

enum { ST_NORMAL = 0, ST_A = 1, ST_B = 2 };
static const uint8_t tac_bit[4] = {9, 3, 5, 7};

static bool signal_now(void) {
    return (t.tac & 0x04) && ((t.counter >> tac_bit[t.tac & 3]) & 1);
}

static void inc_tima(void) {
    if (++t.tima == 0) {       
        t.state = ST_A;
    }
}

void timer_init(Bus *bus) {
    t.bus = bus;
    t.counter = 0xABCC;       
    t.tima = t.tma = 0;
    t.tac = 0;
    t.state = ST_NORMAL;
}

void timer_tick(void) {
    if (t.state == ST_A) {
        t.tima = t.tma;
        t.bus->if_reg |= 0x04;  
        t.state = ST_B;
    } else if (t.state == ST_B) {
        t.state = ST_NORMAL;
    }
    bool before = signal_now();
    t.counter += 4;
    if (before && !signal_now()) inc_tima();
}

uint8_t timer_read(uint16_t a) {
    switch (a) {
    case 0xFF04: return (uint8_t)(t.counter >> 8);
    case 0xFF05: return t.tima;
    case 0xFF06: return t.tma;
    default:     return t.tac | 0xF8;        
    }
}

void timer_write(uint16_t a, uint8_t v) {
    bool before = signal_now();
    switch (a) {
    case 0xFF04:                                
        t.counter = 0;
        break;
    case 0xFF05:
        if (t.state == ST_B) break;           
        t.tima = v;
        if (t.state == ST_A) t.state = ST_NORMAL;   
        break;
    case 0xFF06:
        t.tma = v;
        if (t.state == ST_B) t.tima = v;     
        break;
    default:
        t.tac = v & 7;
        break;
    }
    if (before && !signal_now()) inc_tima();  
}