#!/usr/bin/env python3
"""Suite for SM83 indirect LD opcodes 02,12,0A,1A,22,2A,32,3A.
Run: python3 tests/ld_indirect_suite/test_ld_indirect.py
"""
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
CASES = [
    (90, 'ld_bc_a', '02', [0x01,0x00,0xC1, 0x3E,0x5A, 0x02, 0x3E,0x00, 0x0A]),
    (91, 'ld_de_a', '12', [0x11,0x00,0xC1, 0x3E,0x5A, 0x12, 0x3E,0x00, 0x1A]),
    (92, 'ld_a_bc', '0A', [0x01,0x00,0xC1, 0x3E,0xA6, 0x02, 0x3E,0x00, 0x0A]),
    (93, 'ld_a_de', '1A', [0x11,0x00,0xC1, 0x3E,0xA6, 0x12, 0x3E,0x00, 0x1A]),
    (94, 'ld_hli_a', '22', [0x21,0x00,0xC1, 0x3E,0x5A, 0x22, 0x3E,0x77, 0x77, 0x21,0x00,0xC1, 0x7E, 0x21,0x01,0xC1, 0x7E]),
    (95, 'ld_a_hli', '2A', [0x21,0x00,0xC1, 0x3E,0x5A, 0x77, 0x21,0x01,0xC1, 0x3E,0x77, 0x77, 0x21,0x00,0xC1, 0x2A, 0x7E]),
    (96, 'ld_hld_a', '32', [0x21,0x01,0xC1, 0x3E,0x77, 0x32, 0x3E,0x5A, 0x77, 0x21,0x01,0xC1, 0x7E, 0x21,0x00,0xC1, 0x7E]),
    (97, 'ld_a_hld', '3A', [0x21,0x01,0xC1, 0x3E,0x77, 0x77, 0x21,0x00,0xC1, 0x3E,0x5A, 0x77, 0x21,0x01,0xC1, 0x3A, 0x7E]),
]
TRACE = re.compile(r'^([0-9a-fA-F]{4}):\s+([A-Z][A-Z0-9]*)\s+\(([0-9a-fA-F]{2})\s+[0-9a-fA-F]{2}\s+[0-9a-fA-F]{2}\).*?\nAFTER \| PC:([0-9a-fA-F]{4}) A:([0-9a-fA-F]{2}) B:([0-9a-fA-F]{2}) C:([0-9a-fA-F]{2}) F:([0-9a-fA-F]{2})', re.M)

def get_root():
    for p in [HERE, *HERE.parents]:
        if (p/'src').is_dir() and (p/'include').is_dir():
            return p
    raise RuntimeError('Nao encontrei raiz com src/ e include/. Extraia a pasta dentro do projeto.')

def make_rom(data, path):
    rom=bytearray(32768)
    rom[0x100:0x100+len(data)]=bytes(data)
    path.write_bytes(rom)

def reference(rom):
    mem = bytearray(65536)
    mem[:len(rom)]=rom
    pc=0x100
    a,b,c,d,e,h,l,f=0x01,0x00,0x13,0x00,0xD8,0x01,0x4D,0xB0
    mcycles=0
    trace=[]
    for _ in range(10):
        old_pc=pc
        op=mem[pc]; pc+=1; mcycles+=1
        if op in (0x01,0x11,0x21):
            lo=mem[pc]; hi=mem[pc+1]; pc+=2; mcycles+=2
            if op==0x01: b,c=hi,lo
            elif op==0x11: d,e=hi,lo
            else: h,l=hi,lo
        elif op==0x3E:
            a=mem[pc]; pc+=1; mcycles+=1
        elif op in (0x02,0x0A,0x12,0x1A,0x22,0x2A,0x32,0x3A,0x77,0x7E):
            addr=((b<<8)|c) if op in (0x02,0x0A) else ((d<<8)|e) if op in (0x12,0x1A) else ((h<<8)|l)
            if op in (0x02,0x12,0x22,0x32,0x77): mem[addr]=a
            else: a=mem[addr]
            mcycles+=1
            if op in (0x22,0x2A,0x32,0x3A):
                hl=(((h<<8)|l)+(1 if op in (0x22,0x2A) else -1)) & 0xFFFF
                h,l=hl>>8,hl&0xFF
        elif op!=0x00:
            raise AssertionError(f'reference unsupported opcode {op:02X} at {old_pc:04X}')
        trace.append((old_pc,op,pc,a,b,c,f))
    return trace,mcycles

def main():
    root=get_root()
    print('Raiz do projeto:',root)
    cmd=['clang','-std=c11','-Wall','-Wextra','-Werror=return-type','-Iinclude',*[str(p.relative_to(root)) for p in sorted((root/'src').glob('*.c'))],'-o','gameboy']
    print('Compilando...',end=' ',flush=True)
    res=subprocess.run(cmd,cwd=root,text=True,capture_output=True)
    if res.returncode:
        print('FALHOU\n'+res.stdout+res.stderr)
        return 2
    print('OK')
    success=0
    for number,name,opcode,data in CASES:
        path=HERE/f'{number}_{name}.gb'
        make_rom(data,path)
        rom=path.read_bytes()
        expected,cycles=reference(rom)
        run=subprocess.run([str(root/'gameboy'),str(path)],cwd=root,text=True,capture_output=True)
        output=run.stdout+'\n'+run.stderr
        matches=[(int(m.group(1),16),int(m.group(3),16),int(m.group(4),16),int(m.group(5),16),int(m.group(6),16),int(m.group(7),16),int(m.group(8),16)) for m in TRACE.finditer(output)]
        errors=[]
        if run.returncode: errors.append(f'execucao saiu com codigo {run.returncode}')
        if len(matches)!=10: errors.append(f'esperava 10 instrucoes no log, obteve {len(matches)}')
        for i,(got,exp) in enumerate(zip(matches,expected),1):
            if got!=exp:
                errors.append(f'instrucao {i}: esperado PC/op/PCnovo/A/B/C/F={exp}, obtido={got}')
                break
        m=re.search(r'M-cycles:\s*(\d+)',output)
        if not m or int(m.group(1))!=cycles:
            errors.append(f'M-cycles esperado {cycles}, obtido '+(m.group(1) if m else 'ausente'))
        if errors:
            print(f'FAIL {path.name} opcode={opcode}: '+ '; '.join(errors))
            if len(matches)<10:
                print('  Ultimas linhas:', ' | '.join(output.strip().splitlines()[-5:]))
        else:
            success+=1
            print(f'PASS {path.name} opcode={opcode} PC={expected[-1][2]:04X} A={expected[-1][3]:02X} ciclos={cycles}')
    print(f'\nRESULTADO LD INDIRETO: {success}/{len(CASES)} testes passaram')
    return 0 if success==len(CASES) else 1

if __name__=='__main__':
    sys.exit(main())
