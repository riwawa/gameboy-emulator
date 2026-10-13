
#include "cpu.h"
#include "bus.h"
#include <stdio.h>
#include <string.h>

static cpu_context ctx = {0};

static uint16_t cpu_read_reg(reg_type reg) {
    switch (reg) {
        case RT_A: return ctx.regs.a;
        case RT_F: return ctx.regs.f;
        case RT_B: return ctx.regs.b;
        case RT_C: return ctx.regs.c;
        case RT_D: return ctx.regs.d;
        case RT_E: return ctx.regs.e;
        case RT_H: return ctx.regs.h;
        case RT_L: return ctx.regs.l;

        case RT_AF:
            return (ctx.regs.a << 8) | ctx.regs.f;
        case RT_BC:
            return (ctx.regs.b << 8) | ctx.regs.c;
        case RT_DE:
            return (ctx.regs.d << 8) | ctx.regs.e;
        case RT_HL:
            return (ctx.regs.h << 8) | ctx.regs.l;

        case RT_SP: return ctx.regs.sp;
        case RT_PC: return ctx.regs.pc;

        default:
            return 0;
    }
}

void cpu_init(void) {
    memset(&ctx, 0, sizeof(ctx));
    instructions_init();
}

static void fetch_instruction(void) {
    ctx.current_opcode = bus_read(ctx.regs.pc++);
    ctx.current_instruction = instruction_by_opcode(ctx.current_opcode);
}

static bool fetch_data(void) {
    ctx.fetched_data = 0;
    ctx.mem_dest = 0;
    ctx.dest_is_mem = false;

    if (ctx.current_instruction == NULL) {
        return false;
    }

    switch (ctx.current_instruction->mode) {
        case AM_IMP:
            return true;

        case AM_R:
            ctx.fetched_data =
                cpu_read_reg(ctx.current_instruction->reg_1);
            return true;

        case AM_R_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc++);
            return true;

        case AM_R_D16: {
            uint8_t low = bus_read(ctx.regs.pc++);
            uint8_t high = bus_read(ctx.regs.pc++);

            ctx.fetched_data = (high << 8) | low;
            return true;
        }

        default:
            fprintf(stderr,
                "Addressing mode not implemented: %d\n",
                ctx.current_instruction->mode);
            return false;
    }
}

bool cpu_step(void) {
    if (ctx.halted) {
        return false;
    }

    fetch_instruction();

    if (ctx.current_instruction == NULL ||
        ctx.current_instruction->type == IN_NONE ||
        ctx.current_instruction->type == IN_ERR) {
        fprintf(stderr, "Invalid/unimplemented opcode: 0x%02X\n",
                ctx.current_opcode);
        return false;
    }

    if (!fetch_data()) {
        return false;
    }

    return true;
}
