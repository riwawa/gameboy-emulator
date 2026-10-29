#!/usr/bin/env python3
"""Testes de JP NZ/Z/NC/C com caminhos taken e not-taken, para gb-dmg01."""
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = next((p for p in (HERE, *HERE.parents) if (p / 'src').is_dir() and (p / 'include').is_dir()), None)
CASES = [
    # filename, opcode, A inicial, imediato CP, salta, F depois CP
    ('53_jp_nz_taken.gb',     0xC2, 0x01, 0x00, True,  0x40),
    ('54_jp_nz_not_taken.gb', 0xC2, 0x01, 0x01, False, 0xC0),
    ('55_jp_z_taken.gb',      0xCA, 0x01, 0x01, True,  0xC0),
    ('56_jp_z_not_taken.gb',  0xCA, 0x01, 0x00, False, 0x40),
    ('57_jp_nc_taken.gb',     0xD2, 0x01, 0x00, True,  0x40),
    ('58_jp_nc_not_taken.gb', 0xD2, 0x00, 0x01, False, 0x70),
    ('59_jp_c_taken.gb',      0xDA, 0x00, 0x01, True,  0x70),
    ('60_jp_c_not_taken.gb',  0xDA, 0x01, 0x00, False, 0x40),
]
BEFORE = re.compile(r'^([0-9A-Fa-f]{4}):\s+\S+\s+\(([0-9A-Fa-f]{2})\s+[0-9A-Fa-f]{2}\s+[0-9A-Fa-f]{2}\)', re.M)
AFTER = re.compile(r'^AFTER \| PC:([0-9A-Fa-f]{4}) A:([0-9A-Fa-f]{2}) B:([0-9A-Fa-f]{2}) C:([0-9A-Fa-f]{2}) F:([0-9A-Fa-f]{2})', re.M)

def run_case(name, opcode, a, cp, taken, expected_flags):
    try:
        proc = subprocess.run([str(ROOT/'gameboy'), str(HERE/name)], capture_output=True, text=True, timeout=5)
    except subprocess.TimeoutExpired:
        return 'timeout'
    if proc.returncode != 0:
        return f'executável retornou {proc.returncode}: {proc.stderr[-200:]}'
    befores = BEFORE.findall(proc.stdout)
    afters = AFTER.findall(proc.stdout)
    if len(befores) != 10 or len(afters) != 10:
        return f'esperava 10 linhas de instrução/AFTER, vieram {len(befores)}/{len(afters)}: {proc.stderr[-160:]}'
    addresses = [int(x[0],16) for x in befores]
    opcodes = [int(x[1],16) for x in befores]
    states = [tuple(int(x,16) for x in row) for row in afters] # pc,a,b,c,f
    if addresses[:3] != [0x0100, 0x0102, 0x0104]:
        return f'PCs iniciais incorretos: {[f"{x:04X}" for x in addresses[:3]]}'
    if opcodes[:3] != [0x3E, 0xFE, opcode]:
        return f'opcodes iniciais: {[f"{x:02X}" for x in opcodes[:3]]}; esperado 3E FE {opcode:02X}'
    if states[1][4] != expected_flags:
        return f'CP gerou F={states[1][4]:02X}, esperado {expected_flags:02X}'
    target = 0x0110 if taken else 0x0107
    if states[2][0] != target:
        return f'JP em 0104: PC={states[2][0]:04X}, esperado {target:04X}'
    if states[2][4] != expected_flags:
        return f'JP modificou F: {states[2][4]:02X} != {expected_flags:02X}'
    if addresses[3] != target:
        return f'próxima instrução em {addresses[3]:04X}, esperado {target:04X}'
    expected_b = 0x01 if taken else 0xEF
    expected_f = (expected_flags & 0x10) # INC B zera Z,N,H, preserva C neste caso
    last_pc = 0x0125 if taken else 0x0124
    if states[-1][0] != last_pc:
        return f'PC final {states[-1][0]:04X}, esperado {last_pc:04X}'
    if states[-1][1] != a or states[-1][2] != expected_b or states[-1][4] != expected_f:
        return f'estado final A={states[-1][1]:02X} B={states[-1][2]:02X} F={states[-1][4]:02X}; esperado A={a:02X} B={expected_b:02X} F={expected_f:02X}'
    count = re.search(r'Instructions:\s*(\d+)',proc.stdout)
    cycles = re.search(r'M-cycles:\s*(\d+)',proc.stdout)
    if not count or int(count[1]) != 10 or not cycles or int(cycles[1]) != 18:
        return f'instruções/ciclos incorretos: {count[1] if count else "?"}/{cycles[1] if cycles else "?"} (esperado 10/18)'
    return None

def main():
    if ROOT is None:
        print('FAIL: raiz do projeto não encontrada; coloque a suíte dentro do projeto gb-dmg01')
        return 1
    print(f'Raiz do projeto: {ROOT}')
    sources = sorted((ROOT/'src').glob('*.c'))
    if not sources:
        print('FAIL: nenhum .c encontrado em src/')
        return 1
    cmd = ['clang','-std=c11','-Wall','-Wextra','-Werror=return-type',f'-I{ROOT / "include"}',*[str(p) for p in sources],'-o',str(ROOT/'gameboy')]
    cp = subprocess.run(cmd,capture_output=True,text=True)
    if cp.returncode:
        print('FAIL: compilação\n'+cp.stderr)
        return 1
    print('Compilando... OK')
    passed = 0
    for name,opcode,a,imm,taken,f in CASES:
        err=run_case(name,opcode,a,imm,taken,f)
        if err:
            print(f'FAIL {name}: {err}')
        else:
            passed+=1
            print(f'PASS {name}  JP={"taken" if taken else "not taken"}  B={"01" if taken else "EF"} ciclos=18')
    print(f'\nRESULTADO JP CONDICIONAL: {passed}/{len(CASES)} testes passaram')
    return 0 if passed==len(CASES) else 1

if __name__ == '__main__':
    sys.exit(main())
