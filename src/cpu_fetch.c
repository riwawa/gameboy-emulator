#include <stdio.h>

#include "cpu_internal.h"
#include "emu.h"

// Buscar opcode e operandos

bool cpu_fetch_instruction(cpu_context *cpu) {
    cpu->current_opcode = bus_read(cpu->bus, cpu->regs.pc++);
    emu_cycles(1);
    cpu->current_instruction = instruction_by_opcode(cpu->current_opcode);

    return cpu->current_instruction != NULL;
}

bool cpu_fetch_data(cpu_context *cpu) {
    cpu->fetched_data = 0;
    cpu->mem_dest = 0;
    cpu->dest_is_mem = false;

    if (!cpu->current_instruction) {
        return false;
    }

    switch (cpu->current_instruction->mode) {

        case AM_IMP:
            return true;

        case AM_R:
            cpu->fetched_data = cpu_read_reg(cpu, cpu->current_instruction->reg_1);
            return true;

        case AM_R_R:
            cpu->fetched_data = cpu_read_reg(cpu, cpu->current_instruction->reg_2);
            return true;

        case AM_R_MR: {
            uint16_t address=cpu_read_reg(cpu, cpu->current_instruction->reg_2);
            cpu->fetched_data = bus_read(cpu->bus, address);
            emu_cycles(1);
            return true;
        }

        case AM_MR: {
            cpu->mem_dest = cpu_read_reg(cpu, cpu->current_instruction->reg_1);
            cpu->fetched_data = bus_read(cpu->bus, cpu->mem_dest);

            emu_cycles(1);

            cpu->dest_is_mem = true;

            return true;
        }

        case AM_MR_R: {
            cpu->fetched_data = cpu_read_reg(cpu, cpu->current_instruction->reg_2);
            cpu->mem_dest = cpu_read_reg(cpu, cpu->current_instruction->reg_1);
            cpu->dest_is_mem = true;

            return true;
        }
        case AM_MR_D8: {
            cpu->fetched_data = bus_read(cpu->bus, cpu->regs.pc++);
            emu_cycles(1);
            cpu->mem_dest = cpu_read_reg(cpu, cpu->current_instruction->reg_1);
            cpu->dest_is_mem = true;

            return true;
        }
        case AM_R_D8:
            cpu->fetched_data = bus_read(cpu->bus, cpu->regs.pc++);
            emu_cycles(1);
            return true;

        case AM_R_D16:
        case AM_D16: {
            uint8_t low = bus_read(cpu->bus, cpu->regs.pc++);
            emu_cycles(1);

            uint8_t high = bus_read(cpu->bus, cpu->regs.pc++);
            emu_cycles(1);

            cpu->fetched_data = ((uint16_t)high << 8) | low;

            return true;
        }

        case AM_R_HLI:
        case AM_R_HLD: {
            uint16_t address = cpu_read_reg(cpu, cpu->current_instruction->reg_2);

            cpu->fetched_data = bus_read(cpu->bus, address);

            emu_cycles(1);

            return true;
        }
        case AM_HLD_R:
        case AM_HLI_R: {
            cpu->fetched_data = cpu_read_reg(cpu, cpu->current_instruction->reg_2);
            cpu->mem_dest = cpu_read_reg(cpu, cpu->current_instruction->reg_1);
            cpu->dest_is_mem = true;
            return true;
        }

        case AM_R_A8:
        case AM_A8_R: {
            cpu->fetched_data = bus_read(cpu->bus, cpu->regs.pc++);

            emu_cycles(1);

            return true;
        }

        case AM_JR:
            cpu->fetched_data = bus_read(cpu->bus, cpu->regs.pc++);
            emu_cycles(1);
            return true;

        default:
            fprintf(stderr, "Addressing mode not implemented: %d\n", cpu->current_instruction->mode);
            return false;
    }
}
