
#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "instructions.h"
#include "bus.h"

typedef struct {
    uint8_t a;
    uint8_t f;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t pc;
    uint16_t sp;
} cpu_registers;

typedef struct {
    cpu_registers regs;

    uint16_t fetched_data;
    uint16_t mem_dest;
    uint8_t current_opcode;

    const instruction *current_instruction;
    Bus *bus;
    bool ime;
    uint8_t ime_enable_delay;
    bool stopped;
    bool halt_bug;
    bool dest_is_mem;
    bool halted;
    bool stepping;
} cpu_context;

void cpu_init(Bus *bus);
bool cpu_step(void);
#endif
