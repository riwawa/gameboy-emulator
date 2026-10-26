#include "cpu_internal.h"
#include "emu.h"
uint16_t cpu_read_reg(const cpu_context *cpu, reg_type reg) {
    switch (reg) {
        case RT_A: return cpu->regs.a;
        case RT_F: return cpu->regs.f;
        case RT_B: return cpu->regs.b;
        case RT_C: return cpu->regs.c;
        case RT_D: return cpu->regs.d;
        case RT_E: return cpu->regs.e;
        case RT_H: return cpu->regs.h;
        case RT_L: return cpu->regs.l;

        case RT_AF:
            return (cpu->regs.a << 8) | cpu->regs.f;

        case RT_BC:
            return (cpu->regs.b << 8) | cpu->regs.c;

        case RT_DE:
            return (cpu->regs.d << 8) | cpu->regs.e;

        case RT_HL:
            return (cpu->regs.h << 8) | cpu->regs.l;

        case RT_SP: return cpu->regs.sp;
        case RT_PC: return cpu->regs.pc;

        default: return 0;
    }
}

void cpu_write_reg(cpu_context *cpu, reg_type reg, uint16_t value) {
    switch (reg) {
        case RT_A:
            cpu->regs.a = (uint8_t)value;
            return;

        case RT_F:
            cpu->regs.f = (uint8_t)(value & 0xF0);
            return;

        case RT_B:
            cpu->regs.b = (uint8_t)value;
            return;

        case RT_C:
            cpu->regs.c = (uint8_t)value;
            return;

        case RT_D:
            cpu->regs.d = (uint8_t)value;
            return;

        case RT_E:
            cpu->regs.e = (uint8_t)value;
            return;

        case RT_H:
            cpu->regs.h = (uint8_t)value;
            return;

        case RT_L:
            cpu->regs.l = (uint8_t)value;
            return;

        case RT_AF:
            cpu->regs.a = (uint8_t)(value >> 8);
            cpu->regs.f = (uint8_t)(value & 0xF0);
            return;

        case RT_BC:
            cpu->regs.b = (uint8_t)(value >> 8);
            cpu->regs.c = (uint8_t)value;
            return;

        case RT_DE:
            cpu->regs.d = (uint8_t)(value >> 8);
            cpu->regs.e = (uint8_t)value;
            return;

        case RT_HL:
            cpu->regs.h = (uint8_t)(value >> 8);
            cpu->regs.l = (uint8_t)value;
            return;

        case RT_SP:
            cpu->regs.sp = value;
            return;

        case RT_PC:
            cpu->regs.pc = value;
            return;

        default:
            return;
    }
}
void stack_push16(cpu_context *cpu, uint16_t value) {
    uint8_t high = (uint8_t)(value >> 8);
    uint8_t low  = (uint8_t)(value & 0xFF);

    cpu->regs.sp--;
    bus_write(cpu->bus, cpu->regs.sp, high);
    emu_cycles(1);

    cpu->regs.sp--;
    bus_write(cpu->bus, cpu->regs.sp, low);
    emu_cycles(1);
}
uint16_t stack_pop16(cpu_context *cpu) {
    uint8_t low = bus_read(cpu->bus, cpu->regs.sp++);
    emu_cycles(1);

    uint8_t high = bus_read(cpu->bus, cpu->regs.sp++);
    emu_cycles(1);

    return ((uint16_t)high << 8) | low;
}