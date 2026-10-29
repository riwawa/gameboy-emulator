#!/usr/bin/env python3
"""16 ROMs cobrindo CALL e RET condicionais, com verificação de PC, flags, registradores e ciclos."""
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
CASES = [{'filename': '61_call_nz_taken.gb', 'kind': 'call', 'name': 'NZ', 'taken': True, 'a': 1, 'cp': 0}, {'filename': '62_call_nz_not_taken.gb', 'kind': 'call', 'name': 'NZ', 'taken': False, 'a': 1, 'cp': 1}, {'filename': '63_call_z_taken.gb', 'kind': 'call', 'name': 'Z', 'taken': True, 'a': 1, 'cp': 1}, {'filename': '64_call_z_not_taken.gb', 'kind': 'call', 'name': 'Z', 'taken': False, 'a': 1, 'cp': 0}, {'filename': '65_call_nc_taken.gb', 'kind': 'call', 'name': 'NC', 'taken': True, 'a': 1, 'cp': 0}, {'filename': '66_call_nc_not_taken.gb', 'kind': 'call', 'name': 'NC', 'taken': False, 'a': 0, 'cp': 1}, {'filename': '67_call_c_taken.gb', 'kind': 'call', 'name': 'C', 'taken': True, 'a': 0, 'cp': 1}, {'filename': '68_call_c_not_taken.gb', 'kind': 'call', 'name': 'C', 'taken': False, 'a': 1, 'cp': 0}, {'filename': '69_ret_nz_taken.gb', 'kind': 'ret', 'name': 'NZ', 'taken': True, 'a': 1, 'cp': 0}, {'filename': '70_ret_nz_not_taken.gb', 'kind': 'ret', 'name': 'NZ', 'taken': False, 'a': 1, 'cp': 1}, {'filename': '71_ret_z_taken.gb', 'kind': 'ret', 'name': 'Z', 'taken': True, 'a': 1, 'cp': 1}, {'filename': '72_ret_z_not_taken.gb', 'kind': 'ret', 'name': 'Z', 'taken': False, 'a': 1, 'cp': 0}, {'filename': '73_ret_nc_taken.gb', 'kind': 'ret', 'name': 'NC', 'taken': True, 'a': 1, 'cp': 0}, {'filename': '74_ret_nc_not_taken.gb', 'kind': 'ret', 'name': 'NC', 'taken': False, 'a': 0, 'cp': 1}, {'filename': '75_ret_c_taken.gb', 'kind': 'ret', 'name': 'C', 'taken': True, 'a': 0, 'cp': 1}, {'filename': '76_ret_c_not_taken.gb', 'kind': 'ret', 'name': 'C', 'taken': False, 'a': 1, 'cp': 0}]


def find_root():
    for root in (HERE, *HERE.parents):
        if (root / 'src').is_dir() and (root / 'include').is_dir():
            return root
    raise RuntimeError('Nao foi possivel localizar src/ e include/.')


def expected(case):
    kind, taken, a, cp = case['kind'], case['taken'], case['a'], case['cp']
    # CPU pos-boot: B=00, C=13; CP n preserva A.
    f = 0xC0 if (a, cp) == (1, 1) else (0x40 if (a, cp) == (1, 0) else 0x70)
    pc, b, c, total = 0x0100, 0, 0x13, 0
    traces = []
    for _ in range(10):
        old = pc
        if pc == 0x0100:
            pc = 0x0102; a_now = a; cost = 2
        elif pc == 0x0102:
            pc = 0x0104; cost = 2
        elif pc == 0x0104:
            if kind == 'call' and not taken:
                pc = 0x0107; cost = 3
            else:
                pc = 0x0110; cost = 6
        elif pc == 0x0110:
            if kind == 'call':
                c = 0x55; pc = 0x0112; cost = 2
            elif taken:
                pc = 0x0107; cost = 5
            else:
                pc = 0x0111; cost = 2
        elif pc == 0x0111:
            c = 0x55; pc = 0x0113; cost = 2
        elif pc in (0x0112, 0x0113):
            pc = 0x0107; cost = 4
        elif pc == 0x0107:
            b = 0x44; pc = 0x0109; cost = 2
        elif pc == 0x0109:
            pc = 0x0120; cost = 4
        elif 0x0120 <= pc < 0x0200:
            pc += 1; cost = 1
        else:
            raise AssertionError(f'PC inesperado no modelo: {pc:04X}')
        total += cost
        traces.append((old, pc, a, b, c, 0xB0 if old == 0x0100 else f))
    return traces, total


def parse(output):
    lines = output.splitlines()
    traces = []
    # Nao exige um nome de instrucao especifico, pois debug pode mostrar UNKNOWN.
    head = re.compile(r'^([0-9A-Fa-f]{4}):')
    tail = re.compile(r'^AFTER\s*\|\s*PC:\s*([0-9A-Fa-f]{4})\s+A:\s*([0-9A-Fa-f]{2})\s+B:\s*([0-9A-Fa-f]{2})\s+C:\s*([0-9A-Fa-f]{2})\s+F:\s*([0-9A-Fa-f]{2})')
    last_pc = None
    for line in lines:
        m = head.match(line.strip())
        if m:
            last_pc = int(m.group(1), 16)
            continue
        m = tail.match(line.strip())
        if m and last_pc is not None:
            traces.append((last_pc, *(int(s,16) for s in m.groups())))
            last_pc = None
    mc = re.search(r'M-cycles:\s*(\d+)', output)
    ni = re.search(r'Instructions:\s*(\d+)', output)
    return traces, int(mc.group(1)) if mc else None, int(ni.group(1)) if ni else None


def main():
    root = find_root()
    exe = root / 'gameboy'
    print('Raiz do projeto:', root)
    print('Compilando...', end=' ', flush=True)
    sources = sorted(str(x) for x in (root / 'src').glob('*.c'))
    cmd = ['clang','-std=c11','-Wall','-Wextra','-Werror=return-type','-Iinclude',*sources,'-o',str(exe)]
    build = subprocess.run(cmd, cwd=root, text=True, capture_output=True)
    if build.returncode:
        print('FALHOU\n', build.stdout, build.stderr)
        return 1
    print('OK')
    passed=0
    for case in CASES:
        rom=HERE/case['filename']
        if not rom.is_file():
            print('FAIL', rom.name, ': ROM nao encontrada'); continue
        try:
            run=subprocess.run([str(exe),str(rom)], cwd=root, capture_output=True, text=True, timeout=8)
        except (subprocess.TimeoutExpired, OSError) as exc:
            print('FAIL',rom.name, ':',exc); continue
        if run.returncode:
            print('FAIL',rom.name,f': emulador retornou {run.returncode}',run.stderr[-300:]); continue
        actual,cycles,number=parse(run.stdout)
        want,want_cycles=expected(case)
        failure=None
        if len(actual)!=10: failure=f'{len(actual)} traces, esperado 10'
        else:
            for i,(found,expected_step) in enumerate(zip(actual,want),1):
                if found != expected_step:
                    fmt=lambda t: f'PC {t[0]:04X}->{t[1]:04X} A={t[2]:02X} B={t[3]:02X} C={t[4]:02X} F={t[5]:02X}'
                    failure=f'instrucao {i}: esperado [{fmt(expected_step)}], obtido [{fmt(found)}]'
                    break
        if failure is None and number!=10: failure=f'Instructions={number}, esperado 10'
        if failure is None and cycles!=want_cycles: failure=f'M-cycles={cycles}, esperado {want_cycles}'
        if failure:
            print('FAIL',rom.name,':',failure)
            if len(actual)==0: print('  Final da saida:', ' | '.join(run.stdout.splitlines()[-5:]))
        else:
            passed+=1
            print(f'PASS {rom.name}  {"salta/retorna" if case["taken"] else "nao salta/retorna"}  ciclos={cycles}')
    print(f'\nRESULTADO CALL/RET CONDICIONAL: {passed}/{len(CASES)} testes passaram')
    return 0 if passed==len(CASES) else 1

if __name__=='__main__': sys.exit(main())
