#!/usr/bin/env python3
"""Testes dos 256 opcodes CB da CPU SM83 (duas variantes por opcode)."""
from __future__ import annotations
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = next((p for p in [HERE, *HERE.parents] if (p/'src').is_dir() and (p/'include').is_dir()), None)
if ROOT is None:
    sys.exit('ERRO: coloque cb_suite em tests/ dentro do projeto gb-dmg01.')

TRACE = re.compile(r'^AFTER\s*\|\s*PC:([\da-fA-F]{4})\s+A:([\da-fA-F]{2})\s+B:([\da-fA-F]{2})\s+C:([\da-fA-F]{2})\s+F:([\da-fA-F]{2})', re.M)
CYCLES = re.compile(r'^M-cycles:\s*(\d+)\s*$', re.M)
INSTRUCTIONS = re.compile(r'^Instructions:\s*(\d+)\s*$', re.M)
REGBYTES = [0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, None, 0x3E]


def model(opcode: int, value: int, flags: int):
    x, y = opcode >> 6, (opcode >> 3) & 7
    carry_in = (flags >> 4) & 1
    out, c = value, 0
    if x == 0:
        if y == 0:  # RLC
            c = (value >> 7) & 1; out = ((value << 1) | c) & 0xff
        elif y == 1:  # RRC
            c = value & 1; out = (value >> 1) | (c << 7)
        elif y == 2:  # RL
            c = (value >> 7) & 1; out = ((value << 1) | carry_in) & 0xff
        elif y == 3:  # RR
            c = value & 1; out = (value >> 1) | (carry_in << 7)
        elif y == 4:  # SLA
            c = (value >> 7) & 1; out = (value << 1) & 0xff
        elif y == 5:  # SRA
            c = value & 1; out = (value >> 1) | (value & 0x80)
        elif y == 6:  # SWAP
            out = ((value << 4) | (value >> 4)) & 0xff
        else:  # SRL
            c = value & 1; out = value >> 1
        flags = (0x80 if out == 0 else 0) | (0x10 if c else 0)
    elif x == 1:  # BIT
        flags = (0x80 if not (value & (1 << y)) else 0) | 0x20 | (flags & 0x10)
    elif x == 2:  # RES
        out = value & ~(1 << y)
    else:  # SET
        out = value | (1 << y)
    return out, flags


def case(opcode: int, variation: int):
    x, y, z = opcode >> 6, (opcode >> 3) & 7, opcode & 7
    # Varie especificamente o bit testado/alterado, além das extremidades 0 e 7.
    if x == 1:
        value = (1 << y) if variation else (0xff ^ (1 << y))
    elif x == 2:
        value = 0xff if variation == 0 else (1 << y)
    elif x == 3:
        value = 0x00 if variation == 0 else (0xff ^ (1 << y))
    else:
        values = [0x80, 0x01, 0x81, 0xff, 0x00, 0x7f, 0x55, 0xaa]
        value = values[(y + 3 * variation) % len(values)]
    # Estado inicial após setup: variante 0 F=B0 (Z=1,N=0,H=1,C=1)
    # variante 1 executa CCF e fica F=80 (Z=1,N=0,H=0,C=0).
    flags = 0xB0 if variation == 0 else 0x80
    program = bytearray()
    setup_cycles = 0
    if z == 6:
        program += bytes([0x21, 0x00, 0xC0])  # LD HL,C000
        program += bytes([0x36, value])         # LD (HL),n8
        setup_cycles += 6
    else:
        program += bytes([REGBYTES[z], value])   # LD r,n8
        setup_cycles += 2
    if variation == 1:
        program.append(0x3F)  # CCF: flags passam B0 -> 80
        setup_cycles += 1
    cb_offset = len(program)
    program += bytes([0xCB, opcode])
    probe_offset = len(program)
    program.append(0x78 + z)  # LD A,r, ou LD A,(HL), ou LD A,A
    probe_cycles = 2 if z == 6 else 1
    cb_cycles = (3 if x == 1 else 4) if z == 6 else 2
    pc_cb = 0x0100 + cb_offset + 2
    pc_probe = 0x0100 + probe_offset + 1
    result, new_flags = model(opcode, value, flags)
    while len(program) < 16:
        program.append(0x00)
    rom = bytearray(32768)
    rom[0x100:0x100+len(program)] = program
    suffix = f'{variation}_' + ('mem' if z == 6 else f'r{z}')
    name = f'cb_{opcode:02x}_{suffix}.gb'
    instructions_before = (2 if z == 6 else 1) + variation
    cb_step = instructions_before
    probe_step = cb_step + 1
    nops = 10 - (probe_step + 1)
    if nops < 0:
        raise AssertionError('test program too long')
    total_cycles = setup_cycles + cb_cycles + probe_cycles + nops
    return name, rom, (pc_cb, pc_probe, result, new_flags, cb_step, probe_step, total_cycles, z, value)


def main():
    print(f'Raiz do projeto: {ROOT}')
    binary = HERE/'_gameboy_cb_test'
    sources = sorted((ROOT/'src').glob('*.c'))
    print('Compilando...', end=' ', flush=True)
    build = subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror=return-type', '-I'+str(ROOT/'include'), *map(str,sources), '-o',str(binary)], capture_output=True, text=True)
    if build.returncode:
        print('ERRO')
        print(build.stdout, build.stderr)
        return 1
    print('OK')
    passed = 0
    failures = []
    try:
        for opcode in range(256):
            for variation in (0, 1):
                name, rom, expected = case(opcode, variation)
                path = HERE / name
                path.write_bytes(rom)
                run = subprocess.run([str(binary), str(path)], capture_output=True, text=True, timeout=10)
                matches = [tuple(int(v, 16) for v in m.groups()) for m in TRACE.finditer(run.stdout)]
                c = CYCLES.search(run.stdout)
                n = INSTRUCTIONS.search(run.stdout)
                pc_cb, pc_probe, a_expected, f_expected, cb_step, probe_step, cycles, z, initial = expected
                err = None
                if run.returncode != 0:
                    err = f'exit={run.returncode}, stderr={run.stderr.strip()[:150]}'
                elif len(matches) != 10:
                    err = f'log com {len(matches)} AFTER, esperado 10; stderr={run.stderr.strip()[:100]}'
                elif matches[cb_step][0] != pc_cb:
                    err = f'PC apos CB={matches[cb_step][0]:04X}, esperado {pc_cb:04X}'
                elif matches[cb_step][4] != f_expected:
                    err = f'F apos CB={matches[cb_step][4]:02X}, esperado {f_expected:02X}'
                elif z == 7 and matches[cb_step][1] != a_expected:
                    err = f'A apos CB={matches[cb_step][1]:02X}, esperado {a_expected:02X}'
                elif matches[probe_step][0] != pc_probe:
                    err = f'PC apos leitura={matches[probe_step][0]:04X}, esperado {pc_probe:04X}'
                elif matches[probe_step][1] != a_expected:
                    err = f'A apos leitura={matches[probe_step][1]:02X}, esperado {a_expected:02X}'
                elif matches[probe_step][4] != f_expected:
                    err = f'F apos leitura={matches[probe_step][4]:02X}, esperado {f_expected:02X}'
                elif not n or int(n.group(1)) != 10:
                    err = f'Instructions={n.group(1) if n else "ausente"}, esperado 10'
                elif not c or int(c.group(1)) != cycles:
                    err = f'M-cycles={c.group(1) if c else "ausente"}, esperado {cycles}'
                if err:
                    failures.append((name, f'op={opcode:02X} reg={z} entrada={initial:02X}: {err}', run.stdout))
                    print(f'FAIL {name}: {err}')
                else:
                    passed += 1
                    if variation == 1:
                        print(f'PASS CB {opcode:02X}: 2/2')
    except subprocess.TimeoutExpired as exc:
        print(f'ERRO: emulador excedeu limite na ROM {exc.cmd[-1]}')
        return 1
    finally:
        binary.unlink(missing_ok=True)
    total = 512
    print(f'\nRESULTADO CB-PREFIXED: {passed}/{total} testes passaram')
    if failures:
        print('\nPRIMEIRAS FALHAS (maximo 8):')
        for name, why, out in failures[:8]:
            print(' ',name, why)
            print('  Ultima linha:', out.splitlines()[-1] if out.splitlines() else '<sem saida>')
    return 0 if passed == total else 1

if __name__ == '__main__':
    sys.exit(main())
