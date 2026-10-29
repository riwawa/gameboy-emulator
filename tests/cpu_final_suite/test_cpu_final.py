#!/usr/bin/env python3
"""Testes de integracao SM83: ADD SP, JP HL, IME/interrupts/HALT/STOP.
Gera ROMs de 32 KiB; compila e executa 10 cpu_step() por ROM conforme main atual.
"""
from pathlib import Path
import re, subprocess, sys

HERE = Path(__file__).resolve().parent
ROOT = next((p for p in [HERE, *HERE.parents] if (p/'src').is_dir() and (p/'include').is_dir()), None)
if ROOT is None: sys.exit('ERRO: extraia cpu_final_suite dentro de tests/ na raiz do projeto.')
EXE = ROOT/'gameboy_cpu_final_test'
# Compila uma copia instrumentada do cpu.c, sem alterar os arquivos do usuario.
import tempfile
_tmp = tempfile.TemporaryDirectory(prefix='sm83_suite_')
_cpu_source = (ROOT/'src'/'cpu.c').read_text()
pattern = r'if\s*\(!cpu_execute\(&ctx\)\)\s*\{\s*return false;\s*\}'
trace_code = '''
    printf("TEST_TRACE PC:%04X OP:%02X NEXT:%04X A:%02X F:%02X\\n",
           pc, ctx.current_opcode, ctx.regs.pc, ctx.regs.a, ctx.regs.f);
'''
_cpu_instrumented, n = re.subn(pattern, lambda m: m.group(0) + trace_code, _cpu_source, count=1)
if n != 1:
    sys.exit('ERRO: nao localizei bloco cpu_execute em cpu.c para instrumentar sem editar seu codigo.')
_tmp_cpu = Path(_tmp.name)/'cpu_trace.c'
_tmp_cpu.write_text(_cpu_instrumented)
cmd=['clang','-std=c11','-Wall','-Wextra','-Werror=return-type','-Iinclude','-Isrc',
     *[str(x.relative_to(ROOT)) for x in sorted((ROOT/'src').glob('*.c')) if x.name != 'cpu.c'],
     str(_tmp_cpu), '-o',str(EXE)]
print('Raiz do projeto:',ROOT)
p=subprocess.run(cmd,cwd=ROOT,text=True,capture_output=True)
if p.returncode: sys.exit('ERRO compilando:\n'+p.stdout+p.stderr)
print('Compilando com trace temporario... OK')

# Tests trace matching: list of (PC fetched, opcode, post-PC, A, F) with optional fields None.
# Tests are strictly checked for trace subsequences in order; no guessing when no instruction executes.
def rom(code, patches=None):
    a=bytearray([0x00])*0x8000
    a[0x0100:0x0100+len(code)]=bytes(code)
    for at, chunk in (patches or {}).items(): a[at:at+len(chunk)]=bytes(chunk)
    return a

def E(pc, op, after=None, a=None, f=None): return (pc,op,after,a,f)

# Use ROM code with NOP padding; each case independent. Cycle checks only for cases without halt/interrupt.
tests=[]
def add(name, code, expected, cycles=None, patches=None, min_trace=None):
    tests.append(dict(name=name, code=code, expected=expected, cycles=cycles, patches=patches or {}, min_trace=min_trace or len(expected)))

# ADD SP,e8: SP starts FFFE. Read resulting SP via LD HL,SP+0, then LD A,H and LD A,L.
add('add_sp_positive', [0xE8,0x01,0xF8,0x00,0x7C,0x7D], [E(0x0100,0xE8,0x0102,0x01,0x00), E(0x0102,0xF8,0x0104,0x01,0x00), E(0x0104,0x7C,0x0105,0xFF), E(0x0105,0x7D,0x0106,0xFF)])
add('add_sp_negative', [0xE8,0xFE,0xF8,0x00,0x7C,0x7D], [E(0x0100,0xE8,0x0102,0x01,0x30), E(0x0102,0xF8,0x0104,0x01,0x00), E(0x0104,0x7C,0x0105,0xFF), E(0x0105,0x7D,0x0106,0xFC)])
add('add_sp_zero', [0xE8,0x00,0xF8,0x00,0x7C,0x7D], [E(0x0100,0xE8,0x0102,0x01,0x00), E(0x0102,0xF8,0x0104,0x01,0x00), E(0x0104,0x7C,0x0105,0xFF), E(0x0105,0x7D,0x0106,0xFE)])
# JP HL loads target 0x0120: after must be 0120, and A becomes 0x42.
add('jp_hl', [0x21,0x20,0x01,0xE9,0x3E,0xEE], [E(0x0100,0x21,0x0103), E(0x0103,0xE9,0x0120), E(0x0120,0x3E,0x0122,0x42)], patches={0x0120:[0x3E,0x42]})
# Delayed EI: set IE & IF via writes; EI then NOP then service at 0040; handler LD A,0x42.
set_pending=[0x3E,0x01, 0xEA,0xFF,0xFF, 0xEA,0x0F,0xFF]
add('ei_delay', set_pending+[0xFB,0x00,0x3E,0xEE], [E(0x0108,0xFB,0x0109),E(0x0109,0x00,0x010A),E(0x0040,0x3E,0x0042,0x42)], patches={0x0040:[0x3E,0x42,0xC9]})
# DI cancels pending EI: no interrupt; should execute LD A,0x42 and continue.
add('di_cancels_ei', set_pending+[0xFB,0xF3,0x3E,0x42], [E(0x0108,0xFB,0x0109),E(0x0109,0xF3,0x010A),E(0x010A,0x3E,0x010C,0x42)],patches={0x0040:[0x3E,0x99]})
# DI leaves pending interrupt without serving.
add('di_blocks_interrupt',set_pending+[0xF3,0x3E,0x42], [E(0x0108,0xF3,0x0109),E(0x0109,0x3E,0x010B,0x42)], patches={0x0040:[0x3E,0x99]})
# RETI: CALL 0120 then RETI to 0103; following LD A,0x42.
add('reti_returns', [0xCD,0x20,0x01,0x3E,0x42], [E(0x0100,0xCD,0x0120),E(0x0120,0xD9,0x0103),E(0x0103,0x3E,0x0105,0x42)],patches={0x0120:[0xD9]})
# Interrupt priority: IF=03, IE=03; ISR VBlank 0040 should win over LCD 0048.
set_two=[0x3E,0x03,0xEA,0xFF,0xFF,0xEA,0x0F,0xFF]
add('interrupt_priority',set_two+[0xFB,0x00,0x00],[E(0x0108,0xFB,0x0109),E(0x0109,0x00,0x010A),E(0x0040,0x3E,0x0042,0x40)],patches={0x0040:[0x3E,0x40],0x0048:[0x3E,0x48]})
# HALT bug IME=0 pending=1: HALT consumes once, next INC B repeats.
add('halt_bug',[*set_pending,0x76,0x04,0x78],[E(0x0108,0x76,0x0109),E(0x0109,0x04,0x0109),E(0x0109,0x04,0x010A),E(0x010A,0x78,0x010B,0x02)])
# HALT idle with no pending; after HALT, no more opcodes fetched.
add('halt_idle', [0x76,0x3E,0x42],[E(0x0100,0x76,0x0101)],min_trace=1)
# STOP: should not read following opcode during first 10 steps.
add('stop_state', [0x10,0x00,0x3E,0x42],[E(0x0100,0x10,0x0102)],min_trace=1)

# Tempos totais nos casos sem HALT/STOP ou atendimento de interrupcao.
for t in tests:
    if t['name'].startswith('add_sp_'): t['cycles'] = 15
    if t['name'] == 'jp_hl': t['cycles'] = 13
    if t['name'] == 'reti_returns': t['cycles'] = 19

TRACE = re.compile(r'^TEST_TRACE PC:([0-9A-Fa-f]{4}) OP:([0-9A-Fa-f]{2}) NEXT:([0-9A-Fa-f]{4}) A:([0-9A-Fa-f]{2}) F:([0-9A-Fa-f]{2})', re.M)
CYCLES=re.compile(r'M-cycles:\s*(\d+)')

def parse(s):
    return [tuple(int(v,16) for v in match.groups()) for match in TRACE.finditer(s)]

def check(test, trace, output):
    # Expected required subsequence, preserving order.
    pos=0
    for exp in test['expected']:
        found=False
        while pos<len(trace):
            got=trace[pos];pos+=1
            if got[0]!=exp[0] or got[1]!=exp[1]: continue
            if any(v is not None and got[i]!=v for i,v in enumerate(exp[2:],2)):
                return f'PC {exp[0]:04X} opcode {exp[1]:02X}: observado after={got[2]:04X} A={got[3]:02X} F={got[4]:02X}, esperado after={exp[2]} A={exp[3]} F={exp[4]}'
            found=True;break
        if not found: return f'nao encontrou PC={exp[0]:04X} opcode={exp[1]:02X} no trace na ordem esperada'
    if test['name'] in ('halt_idle','stop_state') and len(trace)>1:
        return 'executou instrucao extra apos entrar em HALT/STOP: '+str(trace[1])
    if test['cycles'] is not None:
        mt=CYCLES.search(output)
        if not mt or int(mt[1])!=test['cycles']:
            return f'M-cycles esperado {test["cycles"]}, obtido {mt[1] if mt else "ausente"}'
    return None

passes=0
for i,t in enumerate(tests,157):
    file=HERE/f'{i:03d}_{t["name"]}.gb'
    file.write_bytes(rom(t['code'],t['patches']))
    try:
        q=subprocess.run([str(EXE),str(file)],cwd=ROOT, text=True, capture_output=True,timeout=5)
    except subprocess.TimeoutExpired:
        print(f'FAIL {file.name}: timeout (possivel loop em HALT/STOP no main)')
        continue
    stdout=q.stdout+'\n'+q.stderr
    trace=parse(stdout)
    err=check(t,trace,stdout)
    if err:
        print(f'FAIL {file.name}: {err}')
        print('  Ultimas linhas:', ' | '.join(stdout.strip().splitlines()[-3:]))
    else:
        print(f'PASS {file.name}  ({len(trace)} instrucoes no trace)')
        passes+=1
print(f'\nRESULTADO CPU FINAL: {passes}/{len(tests)} testes passaram')
if passes!=len(tests): sys.exit(1)
