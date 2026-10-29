#include "cpu_internal.h"
#include "emu.h"
#include <stdlib.h>
#include <stdio.h>

// executar operações

static void cpu_set_flags(cpu_context *cpu, int8_t z, int8_t n, int8_t h, int8_t c) {
    if (z != -1) {
        if (z)
            cpu->regs.f |= 0x80;
        else
            cpu->regs.f &= (uint8_t)~0x80;
    }

    if (n != -1) {
        if (n)
            cpu->regs.f |= 0x40;
        else
            cpu->regs.f &= (uint8_t)~0x40;
    }

    if (h != -1) {
        if (h)
            cpu->regs.f |= 0x20;
        else
            cpu->regs.f &= (uint8_t)~0x20;
    }

    if (c != -1) {
        if (c)
            cpu->regs.f |= 0x10;
        else
            cpu->regs.f &= (uint8_t)~0x10;
    }

    cpu->regs.f &= 0xF0;
}
static void goto_addr(cpu_context *cpu, uint16_t addr) {
    cpu->regs.pc = addr;
    emu_cycles(1);
}
static void cpu_write16(cpu_context *cpu, uint16_t address, uint16_t value) {
    bus_write(cpu->bus, address, (uint8_t)value);
    emu_cycles(1);

    bus_write(cpu->bus, (uint16_t)(address + 1),
              (uint8_t)(value >> 8));
    emu_cycles(1);
}

// LD HL, SP+e8
static bool proc_ld_hl_sp(cpu_context *cpu) {
    uint16_t sp = cpu->regs.sp;
    uint8_t imm = (uint8_t)cpu->fetched_data;
    int8_t offset = (int8_t)imm;

    bool h = ((sp & 0x0F) + (imm & 0x0F)) > 0x0F;
    bool c = ((sp & 0xFF) + imm) > 0xFF;

    uint16_t result = (uint16_t)(sp + offset);

    // Z = 0, N = 0, H e C calculadas acima.
    cpu->regs.f = (h ? 0x20 : 0) |
                  (c ? 0x10 : 0);

    cpu_write_reg(cpu, RT_HL, result);

    // Ciclo interno adicional.
    emu_cycles(1);

    return true;
}

static bool proc_ld(cpu_context *cpu) {
    const instruction *inst = cpu->current_instruction;

    // 1. LD HL,SP+e8 é uma operação especial.
    if (inst->mode == AM_HL_SPR) {
        return proc_ld_hl_sp(cpu);
    }

    // 2. Destino na memória.
    if (cpu->dest_is_mem) {

        // LD [a16],SP escreve dois bytes.
        if (inst->mode == AM_A16_R &&
            inst->reg_2 == RT_SP) {

            cpu_write16(cpu, cpu->mem_dest, cpu->fetched_data);

        } else {
            // Demais escritas LD de um byte.
            bus_write(cpu->bus,cpu->mem_dest, (uint8_t)cpu->fetched_data);

            emu_cycles(1);
        }

    } else {
        // 3. Destino em um registrador.
        cpu_write_reg(cpu, inst->reg_1, cpu->fetched_data);
    }

    // 4. Atualização de HL para HL+ e HL-.
    switch (inst->mode) {
        case AM_R_HLI:
        case AM_HLI_R:
            cpu_write_reg(cpu, RT_HL,
                (uint16_t)(cpu_read_reg(cpu, RT_HL) + 1));
            break;

        case AM_R_HLD:
        case AM_HLD_R:
            cpu_write_reg(cpu, RT_HL,
                (uint16_t)(cpu_read_reg(cpu, RT_HL) - 1));
            break;

        default:
            break;
    }

    // 5. LD SP,HL possui um ciclo interno adicional.
    if (inst->mode == AM_R_R &&
        inst->reg_1 == RT_SP &&
        inst->reg_2 == RT_HL) {
        emu_cycles(1);
    }

    return true;
}
static bool proc_ldh(cpu_context *cpu) {
    const instruction *inst = cpu->current_instruction;

    if (inst->mode == AM_R_A8) {
        // LDH A,[a8]
        uint16_t address = 0xFF00 | (uint8_t)cpu->fetched_data;

        uint8_t value = bus_read(cpu->bus, address);
        emu_cycles(1);

        cpu_write_reg(cpu, RT_A, value);
        return true;
    }

    if (inst->mode == AM_A8_R) {
        // LDH [a8],A
        uint16_t address =
            0xFF00 | (uint8_t)cpu->fetched_data;

        bus_write(cpu->bus, address, cpu->regs.a);
        emu_cycles(1);

        return true;
    }

    return false;
}
static bool proc_nop(cpu_context *cpu) {
    (void)cpu;
    return true;
}

static bool proc_inc(cpu_context *cpu) {
    const instruction *inst = cpu->current_instruction;
    // INC de 16 bits
    if (inst->mode == AM_R &&
        (inst->reg_1 == RT_BC ||
         inst->reg_1 == RT_DE ||
         inst->reg_1 == RT_HL ||
         inst->reg_1 == RT_SP)) {

        uint16_t val = cpu_read_reg(cpu, inst->reg_1);
        cpu_write_reg(cpu, inst->reg_1, (uint16_t)(val + 1));

        emu_cycles(1);
        return true;
    }
    uint8_t anterior;

    if (cpu->dest_is_mem) {
        anterior = (uint8_t)cpu->fetched_data;
    } else {
        anterior = (uint8_t)cpu_read_reg(cpu, inst->reg_1);
    }

    uint8_t val = (uint8_t)(anterior + 1);

    if (cpu->dest_is_mem) {
        bus_write(cpu->bus, cpu->mem_dest, val);
        emu_cycles(1);
    } else {
        cpu_write_reg(cpu, inst->reg_1, val);
    }

    cpu_set_flags(cpu, val == 0, 0, (anterior & 0x0F) == 0x0F, -1);

    return true;
}


static bool proc_dec(cpu_context *cpu) {
    const instruction *inst = cpu->current_instruction;

    if (inst->mode == AM_R &&
        (inst->reg_1 == RT_BC ||
         inst->reg_1 == RT_DE ||
         inst->reg_1 == RT_HL ||
         inst->reg_1 == RT_SP)) {

        uint16_t val = cpu_read_reg(cpu, inst->reg_1);

        cpu_write_reg(cpu, inst->reg_1, (uint16_t)(val - 1));

        emu_cycles(1);
        return true;
    }

    uint8_t anterior;

    if (cpu->dest_is_mem) {
        anterior = (uint8_t)cpu->fetched_data;
    } else {
        anterior = (uint8_t)cpu_read_reg(cpu, inst->reg_1);
    }

    uint8_t val = (uint8_t)(anterior - 1);

    if (cpu->dest_is_mem) {
        bus_write(cpu->bus, cpu->mem_dest, val);
        emu_cycles(1);
    } else {
        cpu_write_reg(cpu, inst->reg_1, val);
    }

    cpu_set_flags(cpu,val == 0,1,(anterior & 0x0F) == 0,-1);

    return true;
}
static bool proc_jp(cpu_context *cpu) {
    goto_addr(cpu, cpu->fetched_data);
    return true;
}
static bool proc_call(cpu_context *cpu) {
    stack_push16(cpu, cpu->regs.pc);

    goto_addr(cpu, cpu->fetched_data);

    return true;
}
static bool proc_rst(cpu_context *cpu) {
    uint16_t return_addr = cpu->regs.pc;
    uint16_t destination = cpu->current_instruction->param;

    emu_cycles(1);

    stack_push16(cpu, return_addr);

    cpu->regs.pc = destination;

    return true;
}
static bool proc_add(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t b = (uint8_t)cpu->fetched_data;

    uint16_t result = (uint16_t)a + b;

    cpu->regs.a = (uint8_t)result;

    cpu_set_flags(cpu,(uint8_t)result == 0,0,((a & 0x0F) + (b & 0x0F)) > 0x0F,result > 0xFF);

    return true;
}
static bool proc_adc(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t b = (uint8_t)cpu->fetched_data;

    uint8_t carry = (cpu->regs.f & 0x10) ? 1 : 0;

    uint16_t result = (uint16_t)a + b + carry;

    cpu->regs.a = (uint8_t)result;

    cpu_set_flags(
        cpu,
        (uint8_t)result == 0,
        0,
        ((a & 0x0F) + (b & 0x0F) + carry) > 0x0F,
        result > 0xFF
    );

    return true;
}
static bool proc_sub(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t b = (uint8_t)cpu->fetched_data;

    uint8_t result = (uint8_t)(a - b);

    cpu->regs.a = result;

    cpu_set_flags(
        cpu,
        result == 0,
        1,
        (a & 0x0F) < (b & 0x0F),
        a < b
    );

    return true;
}
static bool proc_sbc(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t b = (uint8_t)cpu->fetched_data;

    uint8_t carry = (cpu->regs.f & 0x10) ? 1 : 0;

    uint16_t subtrahend = (uint16_t)b + carry;
    uint8_t result = (uint8_t)(a - subtrahend); // A-B-CARRY

    cpu->regs.a = result;

    cpu_set_flags(
        cpu,
        result == 0,
        1,
        (a & 0x0F) < ((b & 0x0F) + carry),
        a < subtrahend
    );

    return true;
}
static bool proc_and(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t b = (uint8_t)cpu->fetched_data;

    uint8_t result = a & b;

    cpu->regs.a = result;

    cpu_set_flags(
        cpu,
        result == 0,
        0,
        1,
        0
    );

    return true;
}
static bool proc_xor(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t b = (uint8_t)cpu->fetched_data;

    uint8_t result = a ^ b;

    cpu->regs.a = result;

    cpu_set_flags(
        cpu,
        result == 0,
        0,
        0,
        0
    );

    return true;
}
static bool proc_or(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t b = (uint8_t)cpu->fetched_data;

    uint8_t result = a | b;

    cpu->regs.a = result;

    cpu_set_flags(
        cpu,
        result == 0,
        0,
        0,
        0
    );

    return true;
}
static bool proc_cp(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t b = (uint8_t)cpu->fetched_data;

    uint8_t result = (uint8_t)(a - b);

    cpu_set_flags(
        cpu,
        result == 0,
        1,
        (a & 0x0F) < (b & 0x0F),
        a < b
    );

    return true;
}
static bool proc_add_hl(cpu_context *cpu) {
    uint16_t hl = cpu_read_reg(cpu, RT_HL);

    uint16_t value = cpu_read_reg(cpu,cpu->current_instruction->reg_2);

    uint32_t result = (uint32_t)hl + value;

    bool half_carry =
        ((hl & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF;

    bool carry = result > 0xFFFF;

    cpu_write_reg(cpu, RT_HL, (uint16_t)result);

    cpu_set_flags(cpu, -1, 0, half_carry, carry);

    emu_cycles(1);

    return true;
}
static bool proc_jr(cpu_context *cpu) {
    int8_t offset = (int8_t)(uint8_t)cpu->fetched_data;

    cpu->regs.pc = (uint16_t)(
        cpu->regs.pc + offset
    );

    emu_cycles(1);

    return true;
}
static bool proc_jr_cond(cpu_context *cpu, bool condition) {
    if (condition) {
        return proc_jr(cpu);
    }

    return true;
}
static bool proc_ret(cpu_context *cpu) {
    uint16_t address = stack_pop16(cpu);
    goto_addr(cpu, address);
    return true;
}
static bool proc_push(cpu_context *cpu) {
    reg_type reg = cpu->current_instruction->reg_1; // qual par de registradores está sendo empilhado
    uint16_t value = cpu_read_reg(cpu, reg);

    emu_cycles(1);

    stack_push16(cpu, value); // Lê os 16 bits desse par.

    return true;
}
static bool proc_pop(cpu_context *cpu) {
    reg_type reg = cpu->current_instruction->reg_1;

    uint16_t value = stack_pop16(cpu);

    cpu_write_reg(cpu, reg, value);

    return true;
}

static bool proc_jp_cond(cpu_context *cpu, bool condition){
    if (condition){
        return proc_jp(cpu);
    }

    return true;        
}
static bool proc_call_cond(cpu_context *cpu, bool condition) {
    if (condition) {
        return proc_call(cpu);
    }

    return true;
}
static bool proc_ret_cond(cpu_context *cpu, bool condition) {
    emu_cycles(1);

    if (condition) {
        return proc_ret(cpu);
    }

    return true;
}
static bool proc_rlca(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t carry = (a >> 7) & 1;

    cpu->regs.a = (uint8_t)((a << 1) | carry);
    cpu_set_flags(cpu, 0, 0, 0, carry);

    return true;
}

static bool proc_rrca(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t carry = a & 1;

    cpu->regs.a = (uint8_t)((a >> 1) | (carry << 7));
    cpu_set_flags(cpu, 0, 0, 0, carry);

    return true;
}

static bool proc_rla(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t old_carry = (cpu->regs.f & 0x10) ? 1 : 0;
    uint8_t new_carry = (a >> 7) & 1;

    cpu->regs.a = (uint8_t)((a << 1) | old_carry);
    cpu_set_flags(cpu, 0, 0, 0, new_carry);

    return true;
}

static bool proc_rra(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t old_carry = (cpu->regs.f & 0x10) ? 1 : 0;
    uint8_t new_carry = a & 1;

    cpu->regs.a = (uint8_t)((a >> 1) | (old_carry << 7));
    cpu_set_flags(cpu, 0, 0, 0, new_carry);

    return true;
}
static bool proc_cpl(cpu_context *cpu) {
    cpu->regs.a = (uint8_t)~cpu->regs.a;

    cpu_set_flags(cpu, -1, 1, 1, -1);

    return true;
}
static bool proc_scf(cpu_context *cpu) {
    cpu_set_flags(cpu, -1, 0, 0, 1);

    return true;
}
static bool proc_ccf(cpu_context *cpu) {
    bool carry = (cpu->regs.f & 0x10) != 0;

    cpu_set_flags(cpu, -1, 0, 0, !carry);

    return true;
}
static bool proc_daa(cpu_context *cpu) {
    uint8_t a = cpu->regs.a;
    uint8_t f = cpu->regs.f;

    bool n = (f & 0x40) != 0;
    bool h = (f & 0x20) != 0;
    bool c = (f & 0x10) != 0;

    uint8_t adjust = 0;
    bool new_carry = c;

    if (!n) {
        // Ajuste após adição.
        if (h || (a & 0x0F) > 9) {
            adjust |= 0x06;
        }

        if (c || a > 0x99) {
            adjust |= 0x60;
            new_carry = true;
        }

        a = (uint8_t)(a + adjust);
    } else {
        // Ajuste após subtração.
        if (h) {
            adjust |= 0x06;
        }

        if (c) {
            adjust |= 0x60;
        }

        a = (uint8_t)(a - adjust);
    }

    cpu->regs.a = a;

    cpu_set_flags(
        cpu,
        a == 0,       // Z
        -1,           // N preservada
        0,            // H sempre zerada
        new_carry     // C
    );

    return true;
}
static bool proc_cb(cpu_context *cpu) {
    // Busca o segundo byte da instrução CB.
    uint8_t opcode = bus_read(cpu->bus, cpu->regs.pc++);
    emu_cycles(1);

    uint8_t x = opcode >> 6;
    uint8_t y = (opcode >> 3) & 7;
    uint8_t z = opcode & 7;

    // Ordem dos registradores na codificação CB.
    static const reg_type regs[8] = {
        RT_B, RT_C, RT_D, RT_E,
        RT_H, RT_L, RT_HL, RT_A
    };

    bool is_mem = (z == 6);
    uint16_t address = 0;
    uint8_t value;

    if (is_mem) {
        address = cpu_read_reg(cpu, RT_HL);
        value = bus_read(cpu->bus, address);
        emu_cycles(1);
    } else {
        value = (uint8_t)cpu_read_reg(cpu, regs[z]);
    }

    uint8_t result = value;
    uint8_t carry = 0;
    uint8_t old_carry = (cpu->regs.f & 0x10) ? 1 : 0;

    switch (x) {
        case 0:
            // Rotações e shifts.
            switch (y) {
                case 0: // RLC
                    carry = value >> 7;
                    result = (uint8_t)((value << 1) | carry);
                    break;

                case 1: // RRC
                    carry = value & 1;
                    result = (uint8_t)((value >> 1) | (carry << 7));
                    break;

                case 2: // RL
                    carry = value >> 7;
                    result = (uint8_t)((value << 1) | old_carry);
                    break;

                case 3: // RR
                    carry = value & 1;
                    result = (uint8_t)((value >> 1) | (old_carry << 7));
                    break;

                case 4: // SLA
                    carry = value >> 7;
                    result = (uint8_t)(value << 1);
                    break;

                case 5: // SRA
                    carry = value & 1;
                    result = (uint8_t)(
                        (value >> 1) | (value & 0x80)
                    );
                    break;

                case 6: // SWAP
                    result = (uint8_t)(
                        (value << 4) | (value >> 4)
                    );
                    carry = 0;
                    break;

                case 7: // SRL
                    carry = value & 1;
                    result = value >> 1;
                    break;
            }

            cpu_set_flags(cpu, result == 0, 0, 0, carry);
            break;

        case 1:
            // BIT y,r: testa o bit sem modificar o valor.
            cpu_set_flags(
                cpu,
                (value & (1u << y)) == 0,
                0,
                1,
                -1
            );
            return true;

        case 2:
            // RES y,r: limpa o bit y.
            result = (uint8_t)(value & ~(1u << y));
            break;

        case 3:
            // SET y,r: ativa o bit y.
            result = (uint8_t)(value | (1u << y));
            break;
    }

    // BIT já retornou
    if (is_mem) {
        bus_write(cpu->bus, address, result);
        emu_cycles(1);
    } else {
        cpu_write_reg(cpu, regs[z], result);
    }

    return true;
}
static bool proc_add_sp(cpu_context *cpu) {
    uint16_t sp = cpu->regs.sp;
    uint8_t imm = (uint8_t)cpu->fetched_data;
    int8_t offset = (int8_t)imm;

    bool h = ((sp & 0x0F) + (imm & 0x0F)) > 0x0F;
    bool c = ((sp & 0xFF) + imm) > 0xFF;

    cpu->regs.sp = (uint16_t)(sp + offset);

    cpu_set_flags(cpu, 0, 0, h, c);
    emu_cycles(2);

    return true;
}

static bool proc_jphl(cpu_context *cpu) {
    cpu->regs.pc = cpu_read_reg(cpu, RT_HL);
    return true;
}
static bool proc_di(cpu_context *cpu) {
    cpu->ime = false;
    cpu->ime_enable_delay = 0;
    return true;
}

static bool proc_ei(cpu_context *cpu) {
    cpu->ime_enable_delay = 2;
    return true;
}

static bool proc_reti(cpu_context *cpu) {
    uint16_t address = stack_pop16(cpu);
    goto_addr(cpu, address);
    cpu->ime = true;
    cpu->ime_enable_delay = 0;
    return true;
}
static bool proc_halt(cpu_context *cpu) {
    uint8_t ie = bus_read(cpu->bus, 0xFFFF);
    uint8_t flags = bus_read(cpu->bus, 0xFF0F);
    uint8_t pending = ie & flags & 0x1F;

    if (!cpu->ime && pending) {
        cpu->halt_bug = true;
        cpu->halted = false;
    } else {
        cpu->halted = true;
    }

    return true;
}
static bool proc_stop(cpu_context *cpu) {
    cpu->stopped = true;
    return true;
}
bool cpu_execute(cpu_context *cpu) {
    if (!cpu || !cpu->current_instruction) {
        return false;
    }
    switch (cpu->current_instruction->type) {
        case IN_CB:
            return proc_cb(cpu);
        case IN_LD:
            return proc_ld(cpu);
        case IN_LDH:
            return proc_ldh(cpu);
        case IN_PUSH:
            return proc_push(cpu);

        case IN_POP:
            return proc_pop(cpu);
            
        case IN_INC:
            return proc_inc(cpu);

        case IN_DEC:
            return proc_dec(cpu);

        case IN_ADD:
            if (cpu->current_instruction->reg_1 == RT_SP)
                return proc_add_sp(cpu);

            if (cpu->current_instruction->reg_1 == RT_HL)
                return proc_add_hl(cpu);

            return proc_add(cpu);

        case IN_JPHL:
            return proc_jphl(cpu);
        case IN_ADC:
            return proc_adc(cpu);

        case IN_SUB:
            return proc_sub(cpu);

        case IN_SBC:
            return proc_sbc(cpu);

        case IN_AND:
            return proc_and(cpu);

        case IN_XOR:
            return proc_xor(cpu);

        case IN_OR:
            return proc_or(cpu);

        case IN_CP:
            return proc_cp(cpu);

        case IN_JP:
            return proc_jp(cpu);

        case IN_JR:
            return proc_jr(cpu);

        case IN_JR_NZ:
            return proc_jr_cond(cpu, !(cpu->regs.f & 0x80));

        case IN_JR_Z:
            return proc_jr_cond(cpu, (cpu->regs.f & 0x80) != 0);

        case IN_JR_NC:
            return proc_jr_cond(cpu, !(cpu->regs.f & 0x10));

        case IN_JR_C:
            return proc_jr_cond(cpu, (cpu->regs.f & 0x10) != 0);

        case IN_CALL:
            return proc_call(cpu);
        case IN_JP_NZ:
            return proc_jp_cond(cpu, !(cpu->regs.f & 0x80));

        case IN_JP_Z:
            return proc_jp_cond(cpu, (cpu->regs.f & 0x80) != 0);

        case IN_JP_NC:
            return proc_jp_cond(cpu, !(cpu->regs.f & 0x10));

        case IN_JP_C:
            return proc_jp_cond(cpu, (cpu->regs.f & 0x10) != 0);

        case IN_CALL_NZ:
            return proc_call_cond(cpu, !(cpu->regs.f & 0x80));

        case IN_CALL_Z:
            return proc_call_cond(cpu, (cpu->regs.f & 0x80) != 0);

        case IN_CALL_NC:
            return proc_call_cond(cpu, !(cpu->regs.f & 0x10));

        case IN_CALL_C:
            return proc_call_cond(cpu, (cpu->regs.f & 0x10) != 0);
        case IN_RET_NZ:
            return proc_ret_cond(cpu, !(cpu->regs.f & 0x80));

        case IN_RET_Z:
            return proc_ret_cond(cpu, (cpu->regs.f & 0x80) != 0);

        case IN_RET_NC:
            return proc_ret_cond(cpu, !(cpu->regs.f & 0x10));

        case IN_RET_C:
            return proc_ret_cond(cpu, (cpu->regs.f & 0x10) != 0);

        // Outras
        case IN_RLCA:
            return proc_rlca(cpu);

        case IN_RRCA:
            return proc_rrca(cpu);

        case IN_RLA:
            return proc_rla(cpu);

        case IN_RRA:
            return proc_rra(cpu);

        case IN_RST:
            return proc_rst(cpu);

        case IN_NOP:
            return proc_nop(cpu);

        case IN_RET:
            return proc_ret(cpu);

        case IN_CPL:
            return proc_cpl(cpu);

        case IN_SCF:
            return proc_scf(cpu);

        case IN_CCF:
            return proc_ccf(cpu);

        case IN_DAA:
            return proc_daa(cpu);
        case IN_DI:
            return proc_di(cpu);

        case IN_EI:
            return proc_ei(cpu);

        case IN_RETI:
            return proc_reti(cpu);

        case IN_HALT:
            return proc_halt(cpu);
        case IN_STOP:
            return proc_stop(cpu);
        default:
            fprintf(stderr, "Instruction not implemented: %s\n",
                    inst_name(cpu->current_instruction->type));
            return false;
    }
}
