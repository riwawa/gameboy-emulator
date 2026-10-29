#!/usr/bin/env python3
"""Testes dos saltos relativos condicionais do SM83 (Game Boy DMG)."""
from pathlib import Path
import subprocess
import re
import sys

SUITE = Path(__file__).resolve().parent
ROOT = next((p for p in [SUITE, *SUITE.parents] if (p / 'src').is_dir() and (p / 'include').is_dir()), None)
CASES = [
    ('45_jr_nz_taken.gb', 'JR NZ', True, 0x40, 0x00),
    ('46_jr_nz_not_taken.gb', 'JR NZ', False, 0xC0, 0x00),
    ('47_jr_z_taken.gb', 'JR Z', True, 0xC0, 0x00),
    ('48_jr_z_not_taken.gb', 'JR Z', False, 0x40, 0x00),
    ('49_jr_nc_taken.gb', 'JR NC', True, 0x40, 0x00),
    ('50_jr_nc_not_taken.gb', 'JR NC', False, 0x70, 0x10),
    ('51_jr_c_taken.gb', 'JR C', True, 0x70, 0x10),
    ('52_jr_c_not_taken.gb', 'JR C', False, 0x40, 0x00),
]
STATE = re.compile(r'AFTER \| PC:([0-9A-Fa-f]+) A:([0-9A-Fa-f]+) B:([0-9A-Fa-f]+) C:([0-9A-Fa-f]+) F:([0-9A-Fa-f]+)')
BEFORE = re.compile(r'^([0-9A-Fa-f]{4}):\s+(\S+)', re.M)

def main():
    if ROOT is None:
        print('ERRO: nao encontrei src/ e include/ acima do runner. Extraia a pasta dentro de gb-dmg01 ou tests/.')
        return 2
    print(f'Raiz do projeto: {ROOT}')
    sources = sorted((ROOT / 'src').glob('*.c'))
    if not sources:
        print('ERRO: sem arquivos C em src/.')
        return 2
    cmd = ['clang', '-std=c11', '-Wall', '-Wextra', '-Werror=return-type', f'-I{ROOT / "include"}', *map(str, sources), '-o', str(ROOT / 'gameboy')]
    build = subprocess.run(cmd, text=True, capture_output=True)
    if build.returncode:
        print('FAIL COMPILACAO\n' + build.stdout + build.stderr)
        return 2
    print('Compilando... OK')
    passed = 0
    for filename, op, taken, flags_at_jr, final_flags in CASES:
        path = SUITE / filename
        if not path.exists():
            print(f'FAIL {filename}: ROM ausente')
            continue
        try:
            p = subprocess.run([str(ROOT / 'gameboy'), str(path)], text=True, capture_output=True, timeout=5)
        except subprocess.TimeoutExpired:
            print(f'FAIL {filename}: timeout')
            continue
        log = p.stdout
        lines = BEFORE.findall(log)
        states = STATE.findall(log)
        errors = []
        if p.returncode:
            errors.append(f'exit={p.returncode} {p.stderr.strip()}')
        if len(lines) != 10 or len(states) != 10:
            errors.append(f'log com {len(lines)} instrucoes e {len(states)} estados AFTER, esperados 10')
        else:
            # 0: LD A,n; 1: CP n; 2: JR cond; ...
            jr_pc, jr_name = lines[2]
            after_jr = tuple(int(v,16) for v in states[2])
            expected_pc = 0x0108 if taken else 0x0106
            if int(jr_pc,16) != 0x0104 or jr_name != 'JR':
                errors.append(f'na instrucao 3 esperava JR em 0104, veio {jr_pc}:{jr_name}')
            if after_jr[0] != expected_pc:
                errors.append(f'PC apos JR={after_jr[0]:04X}, esperado {expected_pc:04X}')
            if after_jr[4] != flags_at_jr:
                errors.append(f'F apos JR={after_jr[4]:02X}, esperado {flags_at_jr:02X}')
            end = tuple(int(v, 16) for v in states[-1])
            expected_end_pc = 0x010F if taken else 0x010E
            expected_b = 0x01 if taken else 0xEF
            if end[0] != expected_end_pc:
                errors.append(f'PC final={end[0]:04X}, esperado {expected_end_pc:04X}')
            if end[2] != expected_b:
                errors.append(f'B final={end[2]:02X}, esperado {expected_b:02X}')
            if end[4] != final_flags:
                errors.append(f'F final={end[4]:02X}, esperado {final_flags:02X}')
        m = re.search(r'M-cycles:\s*(\d+)', log)
        n = re.search(r'Instructions:\s*(\d+)', log)
        if not m or int(m.group(1)) != 14:
            errors.append(f'M-cycles={m.group(1) if m else "ausente"}, esperado 14')
        if not n or int(n.group(1)) != 10:
            errors.append(f'Instructions={n.group(1) if n else "ausente"}, esperado 10')
        if errors:
            print(f'FAIL {filename}: ' + '; '.join(errors))
            print('  Ultimas linhas:', ' | '.join(log.splitlines()[-3:]))
        else:
            passed += 1
            print(f'PASS {filename} salto={"SIM" if taken else "NAO"} PC={expected_pc:04X} B={expected_b:02X} ciclos=14')
    print(f'\nRESULTADO JR CONDICIONAL: {passed}/{len(CASES)} testes passaram')
    return 0 if passed == len(CASES) else 1

if __name__ == '__main__':
    sys.exit(main())
