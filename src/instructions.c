
#include "instructions.h"
#include <stddef.h>
#include <stdint.h>

// - type:  qual operação executar
// - mode:  como buscar os operandos
// - reg_1: primeiro registrador
// - reg_2: segundo registrador

static instruction instructions[0x100] = {

    // Instruções sem operandos explícitos
    [0x00] = {.type = IN_NOP,  .mode = AM_IMP},
    [0x07] = {.type = IN_RLCA, .mode = AM_IMP},
    [0x0F] = {.type = IN_RRCA, .mode = AM_IMP},
    [0x10] = {.type = IN_STOP, .mode = AM_STOP},
    [0x17] = {.type = IN_RLA,  .mode = AM_IMP},
    [0x1F] = {.type = IN_RRA,  .mode = AM_IMP},
    [0x27] = {.type = IN_DAA,  .mode = AM_IMP},
    [0x2F] = {.type = IN_CPL,  .mode = AM_IMP},
    [0x37] = {.type = IN_SCF,  .mode = AM_IMP},
    [0x3F] = {.type = IN_CCF,  .mode = AM_IMP},

    // Controle da CPU
    [0x76] = {.type = IN_HALT, .mode = AM_IMP},
    [0xF3] = {.type = IN_DI,   .mode = AM_IMP},
    [0xFB] = {.type = IN_EI,   .mode = AM_IMP},

    // Salto incondicional
    [0xC3] = {.type = IN_JP, .mode = AM_D16}
};

static const char *const inst_lookup[] = {
    [IN_NONE] = "<NONE>",
    [IN_NOP]  = "NOP",
    [IN_LD]   = "LD",
    [IN_INC]  = "INC",
    [IN_DEC]  = "DEC",
    [IN_RLCA] = "RLCA",
    [IN_ADD]  = "ADD",
    [IN_RRCA] = "RRCA",
    [IN_STOP] = "STOP",
    [IN_RLA]  = "RLA",
    [IN_JR]   = "JR",
    [IN_RRA]  = "RRA",
    [IN_DAA]  = "DAA",
    [IN_CPL]  = "CPL",
    [IN_SCF]  = "SCF",
    [IN_CCF]  = "CCF",
    [IN_RET]    = "RET",
    [IN_RET_NZ] = "RET",
    [IN_RET_Z]  = "RET",
    [IN_RET_NC] = "RET",
    [IN_RET_C]  = "RET",
    [IN_HALT] = "HALT",
    [IN_ADC]  = "ADC",
    [IN_SUB]  = "SUB",
    [IN_SBC]  = "SBC",
    [IN_AND]  = "AND",
    [IN_XOR]  = "XOR",
    [IN_OR]   = "OR",
    [IN_CP]   = "CP",
    [IN_CALL_NZ] = "CALL",
    [IN_CALL_Z]  = "CALL",
    [IN_CALL_NC] = "CALL",
    [IN_CALL_C]  = "CALL",  
    [IN_POP]  = "POP",
    [IN_JP]   = "JP",
    [IN_PUSH] = "PUSH",
    [IN_CB]   = "CB",
    [IN_CALL] = "CALL",
    [IN_RETI] = "RETI",
    [IN_LDH]  = "LDH",
    [IN_JPHL] = "JPHL",
    [IN_DI]   = "DI",
    [IN_EI]   = "EI",
    [IN_RST]  = "RST",
    [IN_ERR]  = "ERR",
    [IN_RLC]  = "RLC",
    [IN_RRC]  = "RRC",
    [IN_RL]   = "RL",
    [IN_RR]   = "RR",
    [IN_SLA]  = "SLA",
    [IN_SRA]  = "SRA",
    [IN_SWAP] = "SWAP",
    [IN_SRL]  = "SRL",
    [IN_BIT]  = "BIT",
    [IN_RES]  = "RES",
    [IN_JP_NZ] = "JP",
    [IN_JP_Z]  = "JP",
    [IN_JP_NC] = "JP",
    [IN_JP_C]  = "JP",
    [IN_JR_NZ] = "JR",
    [IN_JR_Z]  = "JR",
    [IN_JR_NC] = "JR",
    [IN_JR_C]  = "JR",
    [IN_SET]  = "SET"
};

void instructions_init(void) {
    const reg_type regs[8] = {
        RT_B, RT_C, RT_D, RT_E,
        RT_H, RT_L, RT_HL, RT_A
    };
    instructions[0xC0] = (instruction){.type = IN_RET_NZ, .mode = AM_IMP};
    instructions[0xC8] = (instruction){.type = IN_RET_Z,  .mode = AM_IMP};
    instructions[0xD0] = (instruction){.type = IN_RET_NC, .mode = AM_IMP};
    instructions[0xD8] = (instruction){.type = IN_RET_C,  .mode = AM_IMP};
    // Pares de registradores de 16 bits.

    const reg_type regs16[4] = {
        RT_BC, RT_DE, RT_HL, RT_SP
    };

    // pro push e pop
    const reg_type stack_regs[4] = {
        RT_BC, RT_DE, RT_HL, RT_AF
    };

    // INC r8 / INC [HL]
    // Opcodes: 04 0C 14 1C 24 2C 34 3C
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = (uint8_t)(0x04 + (r << 3));

        instructions[opcode] = (instruction) {
            .type = IN_INC,
            .mode = (r == 6) ? AM_MR : AM_R,
            .reg_1 = regs[r]
        };
    }

    // DEC r8 / DEC [HL]
    // Opcodes: 05 0D 15 1D 25 2D 35 3D
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = (uint8_t)(0x05 + (r << 3));

        instructions[opcode] = (instruction) {
            .type = IN_DEC,
            .mode = (r == 6) ? AM_MR : AM_R,
            .reg_1 = regs[r]
        };
    }

    // LD r8,n8 / LD [HL],n8
    // Opcodes: 06 0E 16 1E 26 2E 36 3E
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = (uint8_t)(0x06 + (r << 3));

        instructions[opcode] = (instruction) {
            .type = IN_LD,
            .mode = (r == 6) ? AM_MR_D8 : AM_R_D8,
            .reg_1 = regs[r]
        };
    }

    // LD r8,r8 / LD r8,[HL] / LD [HL],r8
    // Opcodes: 40 a 7F, exceto 76 (HALT)
    for (int dst = 0; dst < 8; dst++) {
        for (int src = 0; src < 8; src++) {

            uint8_t opcode =
                (uint8_t)(0x40 + (dst << 3) + src);

            // 0x76 representa HALT, não LD [HL],[HL].
            if (opcode == 0x76) {
                continue;
            }

            instructions[opcode] = (instruction) {
                .type = IN_LD,

                .mode = (dst == 6) ? AM_MR_R :
                        (src == 6) ? AM_R_MR :
                                     AM_R_R,

                .reg_1 = regs[dst],
                .reg_2 = regs[src]
            };
        }
    }
    // LD (a16),A  -- 0xEA
    instructions[0xEA] = (instruction) {
        .type = IN_LD,
        .mode = AM_A16_R,
        .reg_2 = RT_A
    };

    // LD A,(a16)  -- 0xFA
    instructions[0xFA] = (instruction) {
        .type = IN_LD,
        .mode = AM_R_A16,
        .reg_1 = RT_A
    };

    // LD (a16),SP -- 0x08
    instructions[0x08] = (instruction) {
        .type = IN_LD,
        .mode = AM_A16_R,
        .reg_2 = RT_SP
    };

    // LD (C),A    -- 0xE2
    instructions[0xE2] = (instruction) {
        .type = IN_LD,
        .mode = AM_MC_R,
        .reg_2 = RT_A
    };

    // LD A,(C)    -- 0xF2
    instructions[0xF2] = (instruction) {
        .type = IN_LD,
        .mode = AM_R_MC,
        .reg_1 = RT_A
    };

    // LD SP,HL    -- 0xF9
    instructions[0xF9] = (instruction) {
        .type = IN_LD,
        .mode = AM_R_R,
        .reg_1 = RT_SP,
        .reg_2 = RT_HL
    };

    // LD HL,SP+e8 -- 0xF8
    instructions[0xF8] = (instruction) {
        .type = IN_LD,
        .mode = AM_HL_SPR,
        .reg_1 = RT_HL,
        .reg_2 = RT_SP
    };

    // INC rr / DEC rr (16 bits)
    // INC: 03 13 23 33
    // DEC: 0B 1B 2B 3B
    for (int r = 0; r < 4; r++) {

        uint8_t inc_opcode =
            (uint8_t)(0x03 + (r << 4));

        uint8_t dec_opcode =
            (uint8_t)(0x0B + (r << 4));

        instructions[inc_opcode] = (instruction) {
            .type = IN_INC,
            .mode = AM_R,
            .reg_1 = regs16[r]
        };

        instructions[dec_opcode] = (instruction) {
            .type = IN_DEC,
            .mode = AM_R,
            .reg_1 = regs16[r]
        };
    }

    // LD rr,n16
    // Opcodes: 01 11 21 31
    for (int r = 0; r < 4; r++) {
        uint8_t opcode = (uint8_t)(0x01 + (r << 4));

        instructions[opcode] = (instruction) {
            .type = IN_LD,
            .mode = AM_R_D16,
            .reg_1 = regs16[r]
        };
    }

    // ADD A,r8 / ADD A,[HL]
    // Opcodes: 80 a 87
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = (uint8_t)(0x80 + r);

        instructions[opcode] = (instruction) {
            .type = IN_ADD,
            .mode = (r == 6) ? AM_R_MR : AM_R_R,
            .reg_1 = RT_A,
            .reg_2 = regs[r]
        };
    }
    // ADD SP,e8
    instructions[0xE8] = (instruction){
        .type = IN_ADD,
        .mode = AM_SP_R8,
        .reg_1 = RT_SP
    };
    instructions[0xD9] = (instruction){
        .type = IN_RETI,
        .mode = AM_IMP
    };

    // JP HL
    instructions[0xE9] = (instruction){
        .type = IN_JPHL,
        .mode = AM_IMP
    };
    // ADC A,r8 / ADC A,[HL]
    // Opcodes: 88 a 8F
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = (uint8_t)(0x88 + r);

        instructions[opcode] = (instruction) {
            .type = IN_ADC,
            .mode = (r == 6) ? AM_R_MR : AM_R_R,
            .reg_1 = RT_A,
            .reg_2 = regs[r]
        };
    }

    // SUB A,r8 / SUB A,[HL]
    // Opcodes: 90 a 97
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = (uint8_t)(0x90 + r);

        instructions[opcode] = (instruction) {
            .type = IN_SUB,
            .mode = (r == 6) ? AM_R_MR : AM_R_R,
            .reg_1 = RT_A,
            .reg_2 = regs[r]
        };
    }

    // SBC A,r8 / SBC A,[HL]
    // Opcodes: 98 a 9F
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = (uint8_t)(0x98 + r);

        instructions[opcode] = (instruction) {
            .type = IN_SBC,
            .mode = (r == 6) ? AM_R_MR : AM_R_R,
            .reg_1 = RT_A,
            .reg_2 = regs[r]
        };
    }

    // AND A,r / AND A,[HL]
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = 0xA0 + r;

        instructions[opcode] = (instruction) {
            .type = IN_AND,
            .mode = (r == 6) ? AM_R_MR : AM_R_R,
            .reg_1 = RT_A,
            .reg_2 = regs[r]
        };
    }

    // XOR A,r / XOR A,[HL]
    // Opcodes: A8 a AF
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = (uint8_t)(0xA8 + r);

        instructions[opcode] = (instruction) {
            .type = IN_XOR,
            .mode = (r == 6) ? AM_R_MR : AM_R_R,
            .reg_1 = RT_A,
            .reg_2 = regs[r]
        };
    }

    // OR A,r / OR A,[HL]
    // Opcodes: B0 a B7
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = (uint8_t)(0xB0 + r);

        instructions[opcode] = (instruction) {
            .type = IN_OR,
            .mode = (r == 6) ? AM_R_MR : AM_R_R,
            .reg_1 = RT_A,
            .reg_2 = regs[r]
        };
    }

    // CP A,r / CP A,[HL]
    // Opcodes: B8 a BF
    for (int r = 0; r < 8; r++) {
        uint8_t opcode = (uint8_t)(0xB8 + r);

        instructions[opcode] = (instruction) {
            .type = IN_CP,
            .mode = (r == 6) ? AM_R_MR : AM_R_R,
            .reg_1 = RT_A,
            .reg_2 = regs[r]
        };
    }

    // ADD HL,RR
    for (int r = 0; r < 4; r++) {
        uint8_t opcode = 0x09 + (r << 4);

        instructions[opcode] = (instruction) {
            .type = IN_ADD,
            .mode = AM_IMP,
            .reg_1 = RT_HL,
            .reg_2 = regs16[r]
        };
    }
    for (int i = 0; i < 8; i++) {
        uint8_t opcode = 0xC7 + (i << 3);

        instructions[opcode] = (instruction) {
            .type = IN_RST,
            .mode = AM_IMP,
            .param = i * 8
        };
    }
    // ADD A,n8 — 0xC6
    instructions[0xC6] = (instruction) {
        .type = IN_ADD,
        .mode = AM_R_D8,
        .reg_1 = RT_A
    };

    // ADC A,n8 — 0xCE
    instructions[0xCE] = (instruction) {
        .type = IN_ADC,
        .mode = AM_R_D8,
        .reg_1 = RT_A
    };

    // SUB A,n8 — 0xD6
    instructions[0xD6] = (instruction) {
        .type = IN_SUB,
        .mode = AM_R_D8,
        .reg_1 = RT_A
    };

    // SBC A,n8 — 0xDE
    instructions[0xDE] = (instruction) {
        .type = IN_SBC,
        .mode = AM_R_D8,
        .reg_1 = RT_A
    };

    // AND A,n8 — 0xE6
    instructions[0xE6] = (instruction) {
        .type = IN_AND,
        .mode = AM_R_D8,
        .reg_1 = RT_A
    };

    // XOR A,n8
    instructions[0xEE] = (instruction) {
        .type = IN_XOR,
        .mode = AM_R_D8,
        .reg_1 = RT_A
    };

    // OR A,n8
    instructions[0xF6] = (instruction) {
        .type = IN_OR,
        .mode = AM_R_D8,
        .reg_1 = RT_A
    };

    // CP A,n8
    instructions[0xFE] = (instruction) {
        .type = IN_CP,
        .mode = AM_R_D8,
        .reg_1 = RT_A
    };

    instructions[0x18] = (instruction) {
        .type = IN_JR,
        .mode = AM_JR
    };

    instructions[0x20] = (instruction) {
        .type = IN_JR_NZ,
        .mode = AM_JR
    };

    instructions[0x28] = (instruction) {
        .type = IN_JR_Z,
        .mode = AM_JR
    };

    instructions[0x30] = (instruction) {
        .type = IN_JR_NC,
        .mode = AM_JR
    };

    instructions[0x38] = (instruction) {
        .type = IN_JR_C,
        .mode = AM_JR
    };

    instructions[0xF0] = (instruction) {
        .type = IN_LDH,
        .mode = AM_R_A8,
        .reg_1 = RT_A
    };

    instructions[0xE0] = (instruction) {
        .type = IN_LDH,
        .mode = AM_A8_R,
        .reg_2 = RT_A
    };
    instructions[0xCD] = (instruction) {
        .type = IN_CALL,
        .mode = AM_D16
    };
    instructions[0xC9] = (instruction) {
        .type = IN_RET,
        .mode = AM_IMP
    };
    instructions[0xC2] = (instruction) {
        .type = IN_JP_NZ,
        .mode = AM_D16
    };

    instructions[0xCA] = (instruction) {
        .type = IN_JP_Z,
        .mode = AM_D16
    };

    instructions[0xD2] = (instruction) {
        .type = IN_JP_NC,
        .mode = AM_D16
    };

    instructions[0xDA] = (instruction) {
        .type = IN_JP_C,
        .mode = AM_D16
    };
    instructions[0xC4] = (instruction) {
        .type = IN_CALL_NZ,
        .mode = AM_D16
    };

    instructions[0xCC] = (instruction) {
        .type = IN_CALL_Z,
        .mode = AM_D16
    };

    instructions[0xD4] = (instruction) {
        .type = IN_CALL_NC,
        .mode = AM_D16
    };

    instructions[0xDC] = (instruction) {
        .type = IN_CALL_C,
        .mode = AM_D16
    };
    for (int r = 0; r < 4; r++) {
        uint8_t push_opcode = 0xC5 + (r << 4);
        uint8_t pop_opcode  = 0xC1 + (r << 4);

        instructions[push_opcode] = (instruction) {
            .type = IN_PUSH,
            .mode = AM_IMP,
            .reg_1 = stack_regs[r]
        };

        instructions[pop_opcode] = (instruction) {
            .type = IN_POP,
            .mode = AM_IMP,
            .reg_1 = stack_regs[r]
        };
    }

    instructions[0x02] = (instruction){
        .type = IN_LD, .mode = AM_MR_R,
        .reg_1 = RT_BC, .reg_2 = RT_A
    };

    instructions[0x12] = (instruction){
        .type = IN_LD, .mode = AM_MR_R,
        .reg_1 = RT_DE, .reg_2 = RT_A
    };

    instructions[0x0A] = (instruction){
        .type = IN_LD, .mode = AM_R_MR,
        .reg_1 = RT_A, .reg_2 = RT_BC
    };

    instructions[0x1A] = (instruction){
        .type = IN_LD, .mode = AM_R_MR,
        .reg_1 = RT_A, .reg_2 = RT_DE
    };

    instructions[0x22] = (instruction){
        .type = IN_LD, .mode = AM_HLI_R,
        .reg_1 = RT_HL, .reg_2 = RT_A
    };

    instructions[0x2A] = (instruction){
        .type = IN_LD, .mode = AM_R_HLI,
        .reg_1 = RT_A, .reg_2 = RT_HL
    };

    instructions[0x32] = (instruction){
        .type = IN_LD, .mode = AM_HLD_R,
        .reg_1 = RT_HL, .reg_2 = RT_A
    };

    instructions[0x3A] = (instruction){
        .type = IN_LD, .mode = AM_R_HLD,
        .reg_1 = RT_A, .reg_2 = RT_HL
    };
    instructions[0xCB] = (instruction){
        .type = IN_CB,
        .mode = AM_IMP
    };
}   

instruction *instruction_by_opcode(uint8_t opcode) {
    instruction *inst = &instructions[opcode];

    if (inst->type == IN_NONE) {
        return NULL;
    }

    return inst;
}

const char *inst_name(in_type type) {

    if ((int)type < 0 ||
        (size_t)type >=
            sizeof(inst_lookup) / sizeof(inst_lookup[0])) {
        return "UNKNOWN";
    }

    return inst_lookup[type]
        ? inst_lookup[type]
        : "UNKNOWN";
}
