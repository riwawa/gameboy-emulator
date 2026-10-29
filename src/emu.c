#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "emu.h"
#include "cartridge.h"
#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "joypad.h"
#include "timer.h"
#include "apu.h"

#define FRAME_CYCLES 17556   // ciclos-M por quadro (70224 pontos / 4)

static emu_context ctx = {0};
static Cartridge   cart;
static Bus         bus;

const emu_context *emu_get_context(void) {
    return &ctx;
}

void emu_cycles(uint32_t cpu_cycles) {
    for (uint32_t i = 0; i < cpu_cycles; i++) {
        timer_tick();                              // 1x por ciclo-M
        apu_tick();
        for (int d = 0; d < 4; d++) ppu_tick();    // 4 pontos da PPU por ciclo-M
        ctx.cycles++;
    }
}

bool emu_init(const uint8_t *rom, size_t size) {
    if (!cartridge_init(&cart, rom, size)) return false;

    bus = (Bus){0};
    bus_init(&bus, &cart);
    ppu_init(&bus);
    cpu_init(&bus);
    joypad_init(&bus);
    timer_init(&bus);
    apu_init();

    ctx = (emu_context){0};
    ctx.running = true;
    return true;
}

// Roda a emulação até a PPU terminar um quadro e DEVOLVE o controle a quem chamou.
// Quem chama (a plataforma) decide o que fazer entre os quadros: mostrar, ler botões, esperar.
bool emu_run_frame(void) {
    uint32_t start = ppu_frames();
    uint64_t limit = ctx.cycles + 2 * FRAME_CYCLES;   // com o LCD desligado não há quadros: não prende

    while (ppu_frames() == start && ctx.cycles < limit) {
        if (!cpu_step()) {
            ctx.running = false;
            return false;
        }
        ctx.ticks++;
    }
    apu_flush();                                      // entrega as amostras do quadro à plataforma
    return true;
}

void emu_shutdown(void) {
    ctx.running = false;
    cartridge_free(&cart);
}

const char *emu_title(void) {
    return cart.header.title;
}
