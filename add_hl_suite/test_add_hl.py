#!/usr/bin/env python3
"""Run 8 SM83 ADD HL,rr regression ROMs against ../gameboy."""
import json
import re
import subprocess
import sys
from pathlib import Path

SUITE = Path(__file__).resolve().parent
ROOT = SUITE.parent
BINARY = ROOT / 'gameboy'
PATTERN = re.compile(r'AFTER \| PC:([0-9A-Fa-f]+) A:([0-9A-Fa-f]+) B:([0-9A-Fa-f]+) C:([0-9A-Fa-f]+) F:([0-9A-Fa-f]+)')

def main():
    sources = sorted((ROOT / 'src').glob('*.c'))
    if not sources:
        print('FAIL: No C files found under src/; unzip inside project root.')
        return 1
    cmd = ['clang', '-std=c11', '-Wall', '-Wextra', '-I' + str(ROOT / 'include'), *map(str, sources), '-o', str(BINARY)]
    print('Compilando...')
    build = subprocess.run(cmd, capture_output=True, text=True)
    if build.returncode:
        print(build.stderr)
        return 1
    tests = json.loads((SUITE / 'expected.json').read_text())
    passed = 0
    for case in tests:
        path = SUITE / 'tests' / case['filename']
        try:
            result = subprocess.run([str(BINARY), str(path)], capture_output=True, text=True, timeout=5)
        except subprocess.TimeoutExpired:
            print(f"FAIL {case['filename']}: timeout")
            continue
        if result.returncode:
            print(f"FAIL {case['filename']}: exit {result.returncode}: {result.stderr[:120]}")
            continue
        states = PATTERN.findall(result.stdout)
        cycles = re.search(r'M-cycles:\s*(\d+)', result.stdout)
        count = re.search(r'Instructions:\s*(\d+)', result.stdout)
        if len(states)!=10 or cycles is None or count is None:
            print(f"FAIL {case['filename']}: log incompleto ({len(states)} estados)")
            continue
        pc, a, b, _c, f = [int(x,16) for x in states[-1]]
        observed = {'expected_a':a,'expected_b':b,'expected_f':f,'expected_pc':pc,'expected_cycles':int(cycles.group(1))}
        differences = []
        for key, obtained in observed.items():
            expected = case[key]
            if obtained != expected:
                format_spec = '04X' if key=='expected_pc' else 'd' if key=='expected_cycles' else '02X'
                differences.append(f'{key[9:]}={obtained:{format_spec}} esperado={expected:{format_spec}}')
        if int(count.group(1)) != 10:
            differences.append(f"instruções={count.group(1)} esperado=10")
        if differences:
            print(f"FAIL {case['filename']}: " + ', '.join(differences))
        else:
            passed += 1
            print(f"PASS {case['filename']} HL={case['expected_hl']:04X} F={f:02X} ciclos={observed['expected_cycles']}")
    print(f'\nRESULTADO ADD HL,rr: {passed}/{len(tests)} testes passaram')
    return 0 if passed==len(tests) else 1

if __name__ == '__main__':
    sys.exit(main())
