#ifndef EMU_H
#define EMU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    bool paused;
    bool running;
    uint64_t ticks;
    uint64_t cycles;
} emu_context;

bool emu_init(const uint8_t *rom, size_t size);   
bool emu_run_frame(void);                      
void emu_shutdown(void);
const char *emu_title(void);                      
void emu_cycles(uint32_t cpu_cycles);             
const emu_context *emu_get_context(void);

#endif