#include "timer.h"
#include "apu.h"

// Contador de sistema de 16 bits: sobe 4 por ciclo-M (1 por ciclo-T).
// DIV é só o byte alto dele. O TIMA sobe quando o bit escolhido pelo TAC
// CAI de 1 para 0 (e o timer está ligado).
static struct {
    Bus     *bus;
    uint16_t counter;
    uint8_t  tima, tma, tac;
    uint8_t  state;          // 0 = normal, 1 = ciclo A (TIMA=0, recarga pendente), 2 = ciclo B
} t;

enum { ST_NORMAL = 0, ST_A = 1, ST_B = 2 };

// qual bit do contador cada frequência do TAC observa:
// 00 -> bit 9 (4096 Hz), 01 -> bit 3 (262144 Hz), 10 -> bit 5 (65536 Hz), 11 -> bit 7 (16384 Hz)
static const uint8_t tac_bit[4] = {9, 3, 5, 7};

// "Sinal" que o detector de borda de queda enxerga: (bit escolhido) AND (enable)
static bool signal_now(void) {
    return (t.tac & 0x04) && ((t.counter >> tac_bit[t.tac & 3]) & 1);
}

static void inc_tima(void) {
    if (++t.tima == 0) {        // estourou: fica 0 por um ciclo-M, depois recarrega
        t.state = ST_A;
    }
}

void timer_init(Bus *bus) {
    t.bus = bus;
    t.counter = 0xABCC;         // valor após a boot ROM do DMG (DIV = 0xAB)
    t.tima = t.tma = 0;
    t.tac = 0;
    t.state = ST_NORMAL;
}

void timer_tick(void) {
    // 1) resolve a recarga pendente do ciclo anterior
    if (t.state == ST_A) {
        t.tima = t.tma;
        t.bus->if_reg |= 0x04;  // interrupção do timer (bit 2)
        t.state = ST_B;
    } else if (t.state == ST_B) {
        t.state = ST_NORMAL;
    }
    // 2) avança o contador e procura a borda de queda
    bool before = signal_now();
    bool apu_before = (t.counter >> 12) & 1;
    t.counter += 4;
    if (before && !signal_now()) inc_tima();
    if (apu_before && !((t.counter >> 12) & 1)) apu_div_tick();   // 512 Hz: relógio do sequenciador da APU
}

uint8_t timer_read(uint16_t a) {
    switch (a) {
    case 0xFF04: return (uint8_t)(t.counter >> 8);
    case 0xFF05: return t.tima;
    case 0xFF06: return t.tma;
    default:     return t.tac | 0xF8;           // bits 3-7 leem 1
    }
}

void timer_write(uint16_t a, uint8_t v) {
    bool before = signal_now();
    switch (a) {
    case 0xFF04:                                // qualquer escrita zera o contador todo
        if ((t.counter >> 12) & 1) apu_div_tick();   // zerar derruba o bit 12: gera o evento da APU
        t.counter = 0;
        break;
    case 0xFF05:
        if (t.state == ST_B) break;             // ciclo B: escrita ignorada
        t.tima = v;
        if (t.state == ST_A) t.state = ST_NORMAL;   // ciclo A: cancela a recarga e o IF
        break;
    case 0xFF06:
        t.tma = v;
        if (t.state == ST_B) t.tima = v;        // ciclo B: TIMA copia o TMA novo
        break;
    default:
        t.tac = v & 7;
        break;
    }
    if (before && !signal_now()) inc_tima();    // escrever em DIV/TAC pode gerar um "tick" extra
}
