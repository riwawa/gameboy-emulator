
#include <stdio.h>
#include "emu.h"
#include "cartridge.h"
#include "bus.h"
#include "cpu.h"

static emu_context ctx = {0};

const emu_context *emu_get_context(void) {
    return &ctx;
}

void emu_cycles(uint32_t cpu_cycles) {
    ctx.cycles += cpu_cycles;
}

int emu_run(const char *rom_path) {
    Cartridge cart = {0};

    if (!cartridge_load(&cart, rom_path)) {
        fprintf(stderr, "Failed to load ROM: %s\n", rom_path);
        return 1;
    }

    Bus bus = {0};
    bus_init(&bus, &cart);
    cpu_init(&bus);

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;
    ctx.cycles = 0;

    int status = 0;
    const uint64_t max_steps = 10;

    while (ctx.running && ctx.ticks < max_steps) {
        if (ctx.paused) {
            break;
        }

        if (!cpu_step()) {
            fprintf(stderr, "CPU stopped\n");
            status = 1;
            break;
        }

        ctx.ticks++;
    }

    printf("Instructions: %llu\n",
           (unsigned long long)ctx.ticks);
    printf("M-cycles: %llu\n",
           (unsigned long long)ctx.cycles);

    ctx.running = false;
    cartridge_free(&cart);
    return status;
}
