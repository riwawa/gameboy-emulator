
#include "cpu_internal.h"

#include <stdio.h>
#include <string.h>

static cpu_context ctx = {0};

void cpu_init(Bus *bus) {
    memset(&ctx, 0, sizeof(ctx));

    ctx.bus = bus;

    ctx.regs.pc = 0x0100;
    ctx.regs.sp = 0xFFFE;

    cpu_write_reg(&ctx, RT_AF, 0x01B0);
    cpu_write_reg(&ctx, RT_BC, 0x0013);
    cpu_write_reg(&ctx, RT_DE, 0x00D8);
    cpu_write_reg(&ctx, RT_HL, 0x014D);

    instructions_init();
}

bool cpu_step(void) {
    if (ctx.halted) {
        return false;
    }

    uint16_t pc = ctx.regs.pc;

    if (!cpu_fetch_instruction(&ctx)) {
        fprintf(stderr,
                "Unknown opcode: %02X at %04X\n",
                ctx.current_opcode, pc);
        return false;
    }

    if (!cpu_fetch_data(&ctx)) {
        return false;
    }

    printf("%04X: %-7s (%02X %02X %02X) A: %02X B: %02X C: %02X\n",
           pc,
           inst_name(ctx.current_instruction->type),
           ctx.current_opcode,
           bus_read(ctx.bus, (uint16_t)(pc + 1)),
           bus_read(ctx.bus, (uint16_t)(pc + 2)),
           ctx.regs.a,
           ctx.regs.b,
           ctx.regs.c);

    if (!cpu_execute(&ctx)) {
        return false;
    }
    printf(
        "AFTER | PC:%04X A:%02X B:%02X C:%02X F:%02X\n",
        ctx.regs.pc,
        ctx.regs.a,
        ctx.regs.b,
        ctx.regs.c,
        ctx.regs.f
    );
    return true;
}
