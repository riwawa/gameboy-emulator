#!/usr/bin/env python3
"""Tests CALL a16 + RET with exact per-instruction PC/B/C/F checkpoints."""
import json
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = next((p for p in (HERE, *HERE.parents) if (p / 'src').is_dir() and (p / 'include').is_dir()), None)
if ROOT is None:
    sys.exit('FAIL: não encontrei src/ e include/. Extraia a suíte dentro de gb-dmg01/.')

SOURCES = sorted((ROOT / 'src').glob('*.c'))
if not SOURCES:
    sys.exit('FAIL: src/ não contém arquivos C.')

EXE = ROOT / 'gameboy'
cmd = ['clang', '-std=c11', '-Wall', '-Wextra', '-Werror=return-type', '-I'+str(ROOT/'include'), *(str(s) for s in SOURCES), '-o', str(EXE)]
print('Compilando...')
build = subprocess.run(cmd, capture_output=True, text=True)
if build.returncode:
    sys.exit('FAIL compilação:\n' + build.stderr)

state_re = re.compile(r'^AFTER \| PC:([0-9A-Fa-f]+) A:([0-9A-Fa-f]+) B:([0-9A-Fa-f]+) C:([0-9A-Fa-f]+) F:([0-9A-Fa-f]+)$', re.M)
expected = json.loads((HERE / 'expected.json').read_text())
passed = 0

for name, test in expected.items():
    rom = HERE / (name + '.gb')
    try:
        run = subprocess.run([str(EXE), str(rom)], capture_output=True, text=True, timeout=5)
    except subprocess.TimeoutExpired:
        print(f'FAIL {name}: timeout')
        continue
    output = run.stdout + '\n' + run.stderr
    states = [tuple(int(v,16) for v in m) for m in state_re.findall(output)]
    errors = []
    if run.returncode != 0:
        errors.append(f'exit={run.returncode}')
    if len(states) != 10:
        errors.append(f'{len(states)} estados AFTER (esperados 10)')
    for i, ((pc,a,b,c,f), exp_pc, exp_b, exp_c, exp_f) in enumerate(zip(states,test['pc'],test['b'],test['c'],test['f']),start=1):
        if (pc,b,c,f)!=(exp_pc,exp_b,exp_c,exp_f):
            errors.append(f'passo {i}: PC={pc:04X} B={b:02X} C={c:02X} F={f:02X}; esperado PC={exp_pc:04X} B={exp_b:02X} C={exp_c:02X} F={exp_f:02X}')
            break
    instr = re.search(r'Instructions:\s*(\d+)',output)
    cycles = re.search(r'M-cycles:\s*(\d+)',output)
    if not instr or int(instr.group(1))!=10:
        errors.append('Instructions diferente de 10')
    if not cycles or int(cycles.group(1))!=test['cycles']:
        errors.append(f'M-cycles={cycles.group(1) if cycles else "ausente"} (esperado {test["cycles"]})')
    if errors:
        print(f'FAIL {name}: {"; ".join(errors)}')
        if not states:
            print('  Log:', '\n  '.join(output.strip().splitlines()[-6:]))
    else:
        passed+=1
        print(f'PASS {name} PC={states[-1][0]:04X} B={states[-1][2]:02X} C={states[-1][3]:02X} F={states[-1][4]:02X} ciclos={test["cycles"]}')

print(f'\nRESULTADO CALL/RET: {passed}/{len(expected)} testes passaram')
sys.exit(0 if passed == len(expected) else 1)
