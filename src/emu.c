#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "joypad.h"
#include "emu.h"
#include "cartridge.h"
#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "timer.h"
#include "ui.h"
static emu_context ctx = {0};

const emu_context *emu_get_context(void) {
    return &ctx;
}

void emu_cycles(uint32_t cpu_cycles) {
    for (uint32_t i = 0; i < cpu_cycles; i++) {
        timer_tick();                              // 1x por ciclo-M
        for (int d = 0; d < 4; d++) ppu_tick();    // 4 pontos da PPU por ciclo-M
        ctx.cycles++;
    }
}

int emu_run(const char *rom_path) {
    Cartridge cart = {0};

    if (!cartridge_load(&cart, rom_path)) {
        fprintf(stderr, "Failed to load ROM: %s\n", rom_path);
        return 1;
    }

    Bus bus = {0};
    bus_init(&bus, &cart);
    ppu_init(&bus);
    cpu_init(&bus);
    joypad_init(&bus);
    timer_init(&bus);

    if (!ui_init(cart.header.title)) {
        cartridge_free(&cart);
        return 1;
    }

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;
    ctx.cycles = 0;

    const uint64_t FRAME_CYCLES = 17556;  
    uint32_t last_frame = ppu_frames();
    uint64_t last_present = 0;
    int status = 0;

    while (ctx.running) {
        if (!cpu_step()) {
            fprintf(stderr, "CPU stopped\n");
            status = 1;
            break;
        }
        ctx.ticks++;
        bool new_frame = ppu_frames() != last_frame;
        if (new_frame || ctx.cycles - last_present >= 2 * FRAME_CYCLES) {
            last_frame = ppu_frames();
            last_present = ctx.cycles;
            ui_present();
            if (!ui_poll()) break;
            ui_wait_frame();
        }
    }

    printf("Instructions: %llu\n", (unsigned long long)ctx.ticks);
    printf("M-cycles: %llu\n", (unsigned long long)ctx.cycles);

    ctx.running = false;
    ui_quit();
    cartridge_free(&cart);
    return status;
}