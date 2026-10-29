#!/usr/bin/env python3
"""Standalone SM83 PUSH/POP regression suite for the gb-dmg01 10-step CLI."""
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = next((p for p in [HERE, *HERE.parents] if (p/'src').is_dir() and (p/'include').is_dir()), None)

# (title, opcode bytes, instruction width, m-cycles) — 10 instructions per test.
NOP = ("NOP", [0x00], 1)
def ld_pair(pair, value):
    return (f'LD {pair},{value:04X}', [dict(BC=0x01,DE=0x11,HL=0x21,SP=0x31)[pair], value & 255, value >> 8], 3)
def push(pair):
    return (f'PUSH {pair}', [dict(BC=0xC5,DE=0xD5,HL=0xE5,AF=0xF5)[pair]], 4)
def pop(pair):
    return (f'POP {pair}', [dict(BC=0xC1,DE=0xD1,HL=0xE1,AF=0xF1)[pair]], 3)
def ld_a(source):
    return (f'LD A,{source}', [dict(A=0x7F,B=0x78,C=0x79,D=0x7A,E=0x7B,H=0x7C,L=0x7D)[source]], 1)
def pad(seq):
    assert len(seq) <= 10
    return seq + [NOP] * (10-len(seq))

TESTS = [
    (78,'push_pop_bc',pad([ld_pair('BC',0x1234),push('BC'),ld_pair('BC',0),pop('BC'),ld_a('B'),ld_a('C')])),
    (79,'push_pop_de',pad([ld_pair('DE',0x1234),push('DE'),ld_pair('DE',0),pop('DE'),ld_a('D'),ld_a('E')])),
    (80,'push_pop_hl',pad([ld_pair('HL',0x9ABC),push('HL'),ld_pair('HL',0),pop('HL'),ld_a('H'),ld_a('L')])),
    (81,'push_pop_af',pad([push('AF'),ld_pair('BC',0),pop('AF'),ld_a('B')])),
    (82,'pop_af_masks_low_nibble',pad([ld_pair('BC',0x12FF),push('BC'),pop('AF')])),
    (83,'stack_lifo_nested',pad([ld_pair('BC',0x1234),ld_pair('DE',0x5678),push('BC'),push('DE'),ld_pair('BC',0),ld_pair('DE',0),pop('DE'),pop('BC'),ld_a('D'),ld_a('E')])),
    (84,'cross_pair_de_to_bc',pad([ld_pair('DE',0x4567),push('DE'),ld_pair('BC',0),pop('BC'),ld_a('B'),ld_a('C')])),
    (85,'cross_pair_bc_to_hl',pad([ld_pair('BC',0x89AB),push('BC'),ld_pair('HL',0),pop('HL'),ld_a('H'),ld_a('L')])),
    (86,'cross_pair_hl_to_af_mask',pad([ld_pair('HL',0x7FFF),push('HL'),pop('AF')])),
    (87,'two_pushes_two_pops',pad([ld_pair('BC',0x1357),push('BC'),ld_pair('BC',0x2468),push('BC'),pop('DE'),pop('HL'),ld_a('D'),ld_a('E'),ld_a('H'),ld_a('L')])),
    (88,'af_push_to_bc',pad([push('AF'),ld_pair('BC',0),pop('BC'),ld_a('B'),ld_a('C')])),
    (89,'push_does_not_change_flags',pad([ld_pair('BC',0x5566),push('BC'),pop('DE'),ld_a('D'),ld_a('E')])),
]

# Independent narrow interpreter only for instructions used in these ROMs.
REG_INIT = dict(A=0x01,F=0xB0,B=0,C=0x13,D=0,E=0xD8,H=0x01,L=0x4D,SP=0xFFFE,PC=0x0100)
PAIR = dict(BC=('B','C'),DE=('D','E'),HL=('H','L'),AF=('A','F'))
def oracle(sequence):
    r = dict(REG_INIT)
    ram = {}
    states = []
    total = 0
    for name, data, cycles in sequence:
        before = r['PC']; r['PC'] += len(data); total += cycles
        if name.startswith('LD ') and ',' in name:
            dst, src = name[3:].split(',')
            if dst in ('BC','DE','HL','SP'):
                value = int(src,16)
                if dst=='SP': r['SP']=value
                else:
                    hi,lo=PAIR[dst]
                    r[hi]=value>>8; r[lo]=value&255
            elif dst=='A': r['A']=r[src]
        elif name.startswith('PUSH '):
            pair=name[5:]; h,l=PAIR[pair]
            for val in (r[h],r[l]):
                r['SP']=(r['SP']-1)&0xffff; ram[r['SP']]=val
        elif name.startswith('POP '):
            pair=name[4:]; h,l=PAIR[pair]
            low=ram.get(r['SP'],0); r['SP']=(r['SP']+1)&0xffff
            high=ram.get(r['SP'],0); r['SP']=(r['SP']+1)&0xffff
            r[h]=high; r[l]=low if pair!='AF' else low & 0xf0
        states.append(dict(pc=before,opcode=data[0],pc_after=r['PC'],a=r['A'],b=r['B'],c=r['C'],f=r['F'],sp=r['SP'],d=r['D'],e=r['E'],h=r['H'],l=r['L']))
    return states,total

# Sanity-check expected outcomes independent of the emulator.
assert oracle(TESTS[0][2])[0][4]['a'] == 0x12
assert oracle(TESTS[1][2])[0][5]['a'] == 0x34
assert oracle(TESTS[2][2])[0][5]['a'] == 0xBC
assert oracle(TESTS[4][2])[0][2]['f'] == 0xF0
assert oracle(TESTS[8][2])[0][2]['f'] == 0xF0
assert oracle(TESTS[5][2])[0][-1]['b'] == 0x12

TRACE = re.compile(r'^([0-9a-fA-F]{4}):\s+(\S+)\s+\(([0-9a-fA-F]{2}) [0-9a-fA-F]{2} [0-9a-fA-F]{2}\).*?\nAFTER \| PC:([0-9a-fA-F]{4}) A:([0-9a-fA-F]{2}) B:([0-9a-fA-F]{2}) C:([0-9a-fA-F]{2}) F:([0-9a-fA-F]{2})',re.MULTILINE)
METRIC = re.compile(r'^(Instructions|M-cycles):\s*(\d+)\s*$',re.MULTILINE)

def make_rom(seq, target):
    rom=bytearray(32768)
    data=bytes(b for _,op,_ in seq for b in op)
    rom[0x100:0x100+len(data)]=data
    target.write_bytes(rom)

def main():
    if ROOT is None:
        print('ERRO: coloque push_pop_suite em tests/ dentro de gb-dmg01.')
        return 1
    print('Raiz do projeto:',ROOT)
    print('Compilando...',end=' ',flush=True)
    result=subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror=return-type','-Iinclude',*[str(p) for p in sorted((ROOT/'src').glob('*.c'))],'-o',str(ROOT/'gameboy')],cwd=ROOT,capture_output=True,text=True)
    if result.returncode:
        print('FAIL\n'+result.stdout+result.stderr)
        return 1
    print('OK')
    ok=0
    for number,slug,seq in TESTS:
        filename=f'{number:02d}_{slug}.gb'
        rom=HERE/filename
        make_rom(seq,rom)
        run=subprocess.run([str(ROOT/'gameboy'),str(rom)],cwd=ROOT,text=True,capture_output=True,timeout=10)
        stdout=run.stdout
        expected,cycles=oracle(seq)
        steps=[(int(pc,16),name,int(op,16),int(after,16),int(a,16),int(b,16),int(c,16),int(f,16)) for pc,name,op,after,a,b,c,f in TRACE.findall(stdout)]
        metrics=dict((k,int(v)) for k,v in METRIC.findall(stdout))
        errors=[]
        if run.returncode: errors.append(f'emulador retornou codigo {run.returncode}: {run.stderr.strip()[:200]}')
        if len(steps)!=10: errors.append(f'esperava 10 traces; encontrou {len(steps)}')
        for i,(got,exp) in enumerate(zip(steps,expected),1):
            pc,name,op,after,a,b,c,f=got
            comparisons={'PC inicial':(pc,exp['pc']),'opcode':(op,exp['opcode']),'PC seguinte':(after,exp['pc_after']),'A':(a,exp['a']),'B':(b,exp['b']),'C':(c,exp['c']),'F':(f,exp['f'])}
            for field,(actual,want) in comparisons.items():
                if actual!=want:
                    errors.append(f'instrucao {i} ({seq[i-1][0]}): {field} esperado {want:04X}, obtido {actual:04X}')
                    break
            if errors: break
        if metrics.get('Instructions')!=10: errors.append(f'Instructions esperado 10, obtido {metrics.get("Instructions")}')
        if metrics.get('M-cycles')!=cycles: errors.append(f'M-cycles esperado {cycles}, obtido {metrics.get("M-cycles")}')
        if errors:
            print(f'FAIL {filename}: {errors[0]}')
            if len(steps): print('  Ultima instrucao interpretada:',steps[-1])
            if not steps: print('  Saida:',stdout[-500:])
        else:
            print(f'PASS {filename:<34} A={expected[-1]["a"]:02X} F={expected[-1]["f"]:02X} SP={expected[-1]["sp"]:04X} ciclos={cycles}')
            ok+=1
    print(f'\nRESULTADO PUSH/POP: {ok}/{len(TESTS)} testes passaram')
    print('Nota: SP e memoria da pilha sao inferidos pelos valores recuperados; o log nao expoe SP diretamente.')
    return 0 if ok==len(TESTS) else 1
if __name__=='__main__':
    sys.exit(main())
