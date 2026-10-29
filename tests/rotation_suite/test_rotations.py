#!/usr/bin/env python3
"""Testes independentes de RLCA/RRCA/RLA/RRA para o emulador gb-dmg01."""
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
OPS = {'RLCA': 0x07, 'RRCA': 0x0F, 'RLA': 0x17, 'RRA': 0x1F}
VALUES = (0x00, 0x01, 0x80, 0x81, 0x7F, 0xFF)
TRACE = re.compile(r'^([0-9A-Fa-f]{4}):\s*(\S+).*?\nAFTER \| PC:([0-9A-Fa-f]{4}) A:([0-9A-Fa-f]{2}) B:([0-9A-Fa-f]{2}) C:([0-9A-Fa-f]{2}) F:([0-9A-Fa-f]{2})', re.M)
CYCLES = re.compile(r'^M-cycles:\s*(\d+)\s*$', re.M)
INSTRUCTIONS = re.compile(r'^Instructions:\s*(\d+)\s*$', re.M)

def find_root():
    for path in (HERE, *HERE.parents):
        if (path / 'src').is_dir() and (path / 'include').is_dir():
            return path
    raise RuntimeError('Não encontrei a raiz gb-dmg01 (src/ e include/).')

def rotation(op, a, old_c):
    if op == 'RLCA':
        new_c = (a >> 7) & 1
        return ((a << 1) & 255) | new_c, new_c
    if op == 'RRCA':
        new_c = a & 1
        return (a >> 1) | (new_c << 7), new_c
    if op == 'RLA':
        new_c = (a >> 7) & 1
        return ((a << 1) & 255) | old_c, new_c
    new_c = a & 1
    return (a >> 1) | (old_c << 7), new_c

def cases():
    number = 109
    for op, opcode in OPS.items():
        for a in VALUES:
            for carry in (0, 1):
                name = f'{number:03d}_{op.lower()}_a{a:02x}_c{carry}.gb'
                # CP 00 with A=1 -> C=0, CP 01 with A=0 -> C=1;
                # then LD A,target preserves flags. No dependency on SCF/CCF.
                pre_a, cp_value = (0x00, 0x01) if carry else (0x01, 0x00)
                program = bytes([0x3E, pre_a, 0xFE, cp_value, 0x3E, a, opcode] + [0x00]*6)
                yield number, name, op, opcode, a, carry, pre_a, program
                number += 1

def build_roms():
    for _, name, _, _, _, _, _, program in cases():
        rom = bytearray(32768)
        rom[0x100:0x100+len(program)] = program
        (HERE / name).write_bytes(rom)

def expected_steps(op, a, carry, pre_a):
    # (PC antes, opcode, PC depois, A depois, F depois)
    cp_f = 0x70 if carry else 0x40  # CP 01 with A=0 / CP 00 with A=1
    after_a, after_c = rotation(op, a, carry)
    steps = [
        (0x0100, 0x3E, 0x0102, pre_a, 0xB0),
        (0x0102, 0xFE, 0x0104, pre_a, cp_f),
        (0x0104, 0x3E, 0x0106, a, cp_f),
        (0x0106, OPS[op], 0x0107, after_a, 0x10 if after_c else 0x00),
    ]
    for index in range(6):
        pc = 0x0107 + index
        steps.append((pc, 0x00, pc+1, after_a, 0x10 if after_c else 0x00))
    return steps

def inspect(log, op, a, carry, pre_a, program):
    traces = TRACE.findall(log)
    if len(traces) != 10:
        return f'log com {len(traces)} instruções reconhecidas, esperado 10; detalhe: {log[-350:]!r}'
    expected = expected_steps(op, a, carry, pre_a)
    for idx, (line, want) in enumerate(zip(traces, expected), 1):
        pc_before, mnemonic, pc_after, reg_a, reg_b, reg_c, flags = line
        w_before, w_opcode, w_after, w_a, w_f = want
        observed = (int(pc_before,16), int(pc_after,16), int(reg_a,16), int(reg_b,16), int(reg_c,16), int(flags,16))
        desired = (w_before, w_after, w_a, 0x00, 0x13, w_f)
        if observed != desired:
            return (f'passo {idx} ({op}): PC {observed[0]:04X}->{observed[1]:04X}, '
                    f'A={observed[2]:02X} B={observed[3]:02X} C={observed[4]:02X} F={observed[5]:02X}; '
                    f'esperado PC {desired[0]:04X}->{desired[1]:04X}, '
                    f'A={desired[2]:02X} B={desired[3]:02X} C={desired[4]:02X} F={desired[5]:02X}')
        if idx == 4:
            if mnemonic != op:
                return f'passo 4: debugger mostrou {mnemonic!r}, esperado {op!r}'
        elif idx in (1,3) and mnemonic != 'LD':
            return f'passo {idx}: esperado LD, veio {mnemonic}'
        elif idx == 2 and mnemonic != 'CP':
            return f'passo {idx}: esperado CP, veio {mnemonic}'
        elif idx > 4 and mnemonic != 'NOP':
            return f'passo {idx}: esperado NOP, veio {mnemonic}'
    count = INSTRUCTIONS.search(log)
    if not count or int(count.group(1)) != 10:
        return f'Instructions incorreto: {count.group(1) if count else "ausente"}'
    cy = CYCLES.search(log)
    if not cy or int(cy.group(1)) != 13:
        return f'M-cycles incorreto: {cy.group(1) if cy else "ausente"}, esperado 13'
    return None

def main():
    try:
        root = find_root()
    except RuntimeError as error:
        sys.exit(str(error))
    print(f'Raiz do projeto: {root}', flush=True)
    sources = sorted((root / 'src').glob('*.c'))
    if not sources:
        sys.exit('Não encontrei arquivos src/*.c')
    binary = HERE / '_gameboy_rotations'
    cmd = ['clang','-std=c11','-Wall','-Wextra','-Werror=return-type',f'-I{root / "include"}', *map(str,sources),'-o',str(binary)]
    print('Compilando...', end=' ', flush=True)
    result = subprocess.run(cmd, cwd=root, capture_output=True, text=True)
    if result.returncode:
        sys.exit('FALHA NA COMPILAÇÃO\n' + result.stdout + result.stderr)
    print('OK')
    build_roms()
    passed = 0
    failures = []
    for _, name, op, opcode, a, carry, pre_a, program in cases():
        rom_path = HERE / name
        result = subprocess.run([str(binary), str(rom_path)], cwd=root, capture_output=True, text=True, timeout=10)
        if result.returncode:
            reason = f'processo saiu com código {result.returncode}; {result.stderr[-200:]}'
        else:
            reason = inspect(result.stdout, op, a, carry, pre_a, program)
        if reason:
            failures.append(name)
            print(f'FAIL {name}: {reason}')
        else:
            passed += 1
            print(f'PASS {name} A_final={rotation(op,a,carry)[0]:02X} C_final={rotation(op,a,carry)[1]} ciclos=13')
    total = len(list(cases()))
    print(f'\nRESULTADO ROTAÇÕES: {passed}/{total} testes passaram')
    return 1 if failures else 0

if __name__ == '__main__':
    sys.exit(main())
