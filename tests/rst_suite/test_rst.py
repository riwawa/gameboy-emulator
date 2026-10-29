#!/usr/bin/env python3
"""RST instruction tests for the DMG-01 SM83 emulator.
Run: python3 tests/rst_suite/test_rst.py
Can also live at project root: rst_suite/test_rst.py
"""
from pathlib import Path
import re
import subprocess
import sys

SUITE = Path(__file__).resolve().parent


def find_root():
    for folder in [SUITE, *SUITE.parents]:
        if (folder / 'src').is_dir() and (folder / 'include').is_dir():
            return folder
    raise RuntimeError('Nao encontrei src/ e include/ nos diretorios pais. Extraia a suite dentro de gb-dmg01/.')


ROOT = find_root()
EXECUTABLE = ROOT / 'gameboy'
TRACE_RE = re.compile(
    r'^([0-9A-Fa-f]{4}):\s+([A-Z][A-Z0-9 ]*)\s+\([^\n]*\).*?\n'
    r'AFTER \| PC:([0-9A-Fa-f]{4}) A:([0-9A-Fa-f]{2}) '
    r'B:([0-9A-Fa-f]{2}) C:([0-9A-Fa-f]{2}) F:([0-9A-Fa-f]{2})',
    re.MULTILINE
)


def make_rom(vector, opcode):
    rom = bytearray(32768)
    # Main program: RST vector; INC B; subsequent NOPs (zero-filled ROM).
    rom[0x0100] = opcode
    rom[0x0101] = 0x04  # INC B
    # Routine at vector: INC C; RET.
    rom[vector] = 0x0C
    rom[vector + 1] = 0xC9
    return bytes(rom)


def compile_emulator():
    sources = sorted((ROOT / 'src').glob('*.c'))
    if not sources:
        raise RuntimeError('Nenhum arquivo .c encontrado em src/')
    cmd = ['clang', '-std=c11', '-Wall', '-Wextra', '-Werror=return-type',
           '-I' + str(ROOT / 'include'), *map(str, sources), '-o', str(EXECUTABLE)]
    result = subprocess.run(cmd, text=True, capture_output=True)
    if result.returncode:
        print(result.stdout + result.stderr)
        raise RuntimeError('Compilacao falhou')
    if result.stderr.strip():
        print(result.stderr.strip())


def check_case(index):
    vector = index * 8
    opcode = 0xC7 + index * 8
    name = f'{37 + index:02d}_rst_{vector:02X}.gb'
    path = SUITE / name
    expected_rom = make_rom(vector, opcode)
    # Always regenerate so the test suite is self-contained and deterministic.
    path.write_bytes(expected_rom)

    try:
        proc = subprocess.run([str(EXECUTABLE), str(path)], text=True,
                              capture_output=True, timeout=5)
    except subprocess.TimeoutExpired:
        return name, ['timeout de 5 segundos']
    if proc.returncode != 0:
        return name, [f'emulador terminou com codigo {proc.returncode}: {proc.stderr.strip()}']

    trace = TRACE_RE.findall(proc.stdout)
    # Opcode address; instruction mnemonic; next PC; A, B, C, F.
    expected = [
        (0x0100, 'RST', vector, 0x01, 0x00, 0x13, 0xB0),
        (vector, 'INC', vector + 1, 0x01, 0x00, 0x14, 0x10),
        (vector + 1, 'RET', 0x0101, 0x01, 0x00, 0x14, 0x10),
        (0x0101, 'INC', 0x0102, 0x01, 0x01, 0x14, 0x10),
    ]
    errors = []
    if len(trace) != 10:
        errors.append(f'trace tem {len(trace)} instrucoes; esperadas 10')
    for i, want in enumerate(expected):
        if i >= len(trace):
            break
        rec = trace[i]
        got = (int(rec[0], 16), rec[1].strip(),
               *(int(v, 16) for v in rec[2:]))
        if got != want:
            errors.append(f'passo {i + 1}: obtido {got}, esperado {want}')

    if trace:
        last = trace[-1]
        final = (int(last[2], 16), int(last[3], 16), int(last[4], 16),
                 int(last[5], 16), int(last[6], 16))
        final_expected = (0x0108, 0x01, 0x01, 0x14, 0x10)
        if final != final_expected:
            errors.append(f'estado final {final}, esperado {final_expected}')

    m_ins = re.search(r'^Instructions:\s*(\d+)', proc.stdout, re.MULTILINE)
    m_cyc = re.search(r'^M-cycles:\s*(\d+)', proc.stdout, re.MULTILINE)
    if not m_ins or int(m_ins.group(1)) != 10:
        errors.append('Instructions deveria ser 10')
    if not m_cyc or int(m_cyc.group(1)) != 16:
        errors.append(f'M-cycles deveria ser 16 (obtido {m_cyc.group(1) if m_cyc else "ausente"})')

    return name, errors


def main():
    print(f'Raiz do projeto: {ROOT}')
    print('Compilando...')
    compile_emulator()
    ok_count = 0
    for i in range(8):
        name, errors = check_case(i)
        if errors:
            print(f'FAIL {name}')
            for err in errors:
                print('   -', err)
        else:
            ok_count += 1
            print(f'PASS {name}  destino={i * 8:04X} retorno=0101 B=01 C=14 F=10 ciclos=16')
    print(f'\nRESULTADO RST: {ok_count}/8 testes passaram')
    return 0 if ok_count == 8 else 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (RuntimeError, OSError) as exc:
        print('ERRO:', exc, file=sys.stderr)
        sys.exit(1)
