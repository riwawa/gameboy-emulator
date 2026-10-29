#include "cpu_internal.h"

#include <stdio.h>
#include <string.h>
#include "emu.h"

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
    ctx.ime = false;
    ctx.ime_enable_delay = 0;
    ctx.halted = false;
    ctx.stopped = false;
    ctx.halt_bug = false;
    instructions_init();
}
static bool cpu_service_interrupt(void) {
    uint8_t ie = bus_read(ctx.bus, 0xFFFF);
    uint8_t if_reg = bus_read(ctx.bus, 0xFF0F);

    uint8_t pending = ie & if_reg & 0x1F;

    if (!pending) {
        return false;
    }

    ctx.halted = false;

    if (!ctx.ime) {
        return false;
    }

    static const uint16_t vectors[5] = {
        0x0040,  // VBlank
        0x0048,  // LCD STAT
        0x0050,  // Timer
        0x0058,  // Serial
        0x0060   // Joypad
    };

    for (int i = 0; i < 5; i++) {
        if (pending & (1u << i)) {
            ctx.ime = false;

            bus_write(
                ctx.bus,
                0xFF0F,
                (uint8_t)(if_reg & ~(1u << i))
            );

            emu_cycles(2);
            stack_push16(&ctx, ctx.regs.pc);
            ctx.regs.pc = vectors[i];
            emu_cycles(1);

            return true;
        }
    }

    return false;
}
bool cpu_step(void) {
    if (ctx.stopped) {
        return true;
    }

    if (cpu_service_interrupt()) {
        return true;
    }

    if (ctx.halted) {
        emu_cycles(1);
        return true;
    }


    if (ctx.halted) {
        emu_cycles(1);
        return true;
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

    if (!cpu_execute(&ctx)) {
        return false;
    }

    // Atualiza o atraso de EI após executar a instrução.
    if (ctx.ime_enable_delay > 0) {
        ctx.ime_enable_delay--;

        if (ctx.ime_enable_delay == 0) {
            ctx.ime = true;
        }
    }

    return true;
}