#include "instructions.h"
#include <cpu.h>


instruction instructions[0x100] = {
    [0x00] = {IN_NOP, AM_IMP},
    [0x07] = {IN_RLCA, AM_IMP},
    [0x0F] = {IN_RRCA, AM_IMP},
    [0x10] = {IN_STOP, AM_IMP},

    [0x76] = {IN_HALT, AM_IMP},
    [0xC3] = {IN_JP, AM_D16},
    [0xF3] = {IN_DI, AM_IMP},
    [0xFB] = {IN_EI, AM_IMP}
};

void instructions_init(void) {
    reg_type regs[] = {
        RT_B, RT_C, RT_D, RT_E,
        RT_H, RT_L, RT_HL, RT_A
    };
    // INC r
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = 0x04 + (r << 3);
        instructions[opcode] = (instruction) {
            .type = IN_INC,
            .mode = (r == 6) ? AM_MR : AM_R,
            .reg_1 = regs[r]
        };
    }

    // DEC r
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = 0x05 + (r << 3);
        instructions[opcode] = (instruction) {
            .type = IN_DEC,
            .mode = (r == 6) ? AM_MR : AM_R,
            .reg_1 = regs[r]
        };     

    }

    // LD r, n8
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = 0x06 + (r << 3);
        instructions[opcode] = (instruction) {
            .type = IN_LD,
            .mode = (r == 6) ? AM_MR_D8 : AM_R_D8,
            .reg_1 = regs[r]
        };
    }
    
    // LD r, r
    for (int dst = 0; dst < 8; dst++) {
        for (int src = 0; src < 8; src++) {
            uint8_t opcode = 0x40 + (dst << 3) + src;

            if (opcode == 0x76) {
                continue; // HALT
            }

            instructions[opcode] = (instruction) {
                .type = IN_LD,
                .mode = (dst == 6) ? AM_MR_R :
                        (src == 6) ? AM_R_MR : AM_R_R,
                .reg_1 = regs[dst],
                .reg_2 = regs[src]
            };
        }
    }
    // Famílias aritméticas e lógicas
    // ADD, ADC, SUB, SBC, AND, XOR, OR, CP

    // Famílias de registradores de 16 bits
    // LD rr,n16; INC rr; DEC rr; ADD HL,rr

    // Pilha
    // PUSH rr; POP rr

    // RST
}

instruction *instruction_by_opcode(uint8_t opcode) {
    return &instructions[opcode];
}

char *inst_lookup[] = {
    "<NONE>",
    "NOP",
    "LD",
    "INC",
    "DEC",
    "RLCA",
    "ADD",
    "RRCA",
    "STOP",
    "RLA",
    "JR",
    "RRA",
    "DAA",
    "CPL",
    "SCF",
    "CCF",
    "HALT",
    "ADC",
    "SUB",
    "SBC",
    "AND",
    "XOR",
    "OR",
    "CP",
    "POP",
    "JP",
    "PUSH",
    "RET",
    "CB",
    "CALL",
    "RETI",
    "LDH",
    "JPHL",
    "DI",
    "EI",
    "RST",
    "IN_ERR",
    "IN_RLC", 
    "IN_RRC",
    "IN_RL", 
    "IN_RR",
    "IN_SLA", 
    "IN_SRA",
    "IN_SWAP", 
    "IN_SRL",
    "IN_BIT", 
    "IN_RES", 
    "IN_SET"
};

char *inst_name(in_type t) {
    return inst_lookup[t];
}