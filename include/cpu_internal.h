
#ifndef CPU_INTERNAL_H
#define CPU_INTERNAL_H

#include "cpu.h"

bool cpu_fetch_instruction(cpu_context *cpu);
bool cpu_fetch_data(cpu_context *cpu);

uint16_t cpu_read_reg(
    const cpu_context *cpu,
    reg_type reg
);

void cpu_write_reg(
    cpu_context *cpu,
    reg_type reg,
    uint16_t value
);

bool cpu_execute(cpu_context *cpu);
void stack_push16(cpu_context *cpu, uint16_t value);
uint16_t stack_pop16(cpu_context *cpu);
void cpu_write_reg(cpu_context *cpu, reg_type reg, uint16_t value
);

#endif
