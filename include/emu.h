#ifndef EMU_H
#define EMU_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool paused;
    bool running;
    uint64_t ticks;
    uint64_t cycles;
} emu_context;

int emu_run(const char *rom_path);
void emu_cycles(uint32_t cpu_cycles);
const emu_context *emu_get_context(void);

#endif
