#!/usr/bin/env python3
"""Testes SM83 CPL, SCF, CCF, DAA. ROMs de 32 KiB; emulador roda 10 instruções."""
from pathlib import Path
import re, subprocess, sys

HERE = Path(__file__).resolve().parent

def root_find():
    for p in (HERE, *HERE.parents):
        if (p/'src').is_dir() and (p/'include').is_dir(): return p
    raise RuntimeError('Nao achei src/ e include/ na hierarquia do runner.')

TRACE = re.compile(r'^([0-9a-fA-F]{4}):\s*([A-Z][A-Z0-9]*)[^\n]*\nAFTER \| PC:([0-9a-fA-F]{4}) A:([0-9a-fA-F]{2}) B:([0-9a-fA-F]{2}) C:([0-9a-fA-F]{2}) F:([0-9a-fA-F]{2})', re.M)
COUNT = re.compile(r'Instructions:\s*(\d+)')
CYCLES = re.compile(r'M-cycles:\s*(\d+)')

def bcd(d): return (d//10)*16 + d%10

def runner():
    root=root_find()
    binary=HERE/'_gameboy_special_test'
    print('Raiz do projeto:', root)
    print('Compilando...', end=' ',flush=True)
    cmd=['clang','-std=c11','-Wall','-Wextra','-Werror=return-type','-Iinclude', *[str(p) for p in sorted((root/'src').glob('*.c'))],'-o',str(binary)]
    build=subprocess.run(cmd,cwd=root,capture_output=True,text=True)
    if build.returncode:
        print('FAIL\n'+build.stdout+build.stderr)
        return 1
    print('OK')
    cases=make_cases()
    passed=0
    for n, (label, ops) in enumerate(cases,157):
        name=f'{n:03}_{label}.gb'
        rom=bytearray(32768)
        pc=0x100
        for opcode, imm in ops:
            data=bytes((opcode,))+bytes(imm)
            rom[pc:pc+len(data)]=data
            pc+=len(data)
        path=HERE/name
        path.write_bytes(rom)
        result=subprocess.run([str(binary),str(path)],cwd=root,capture_output=True,text=True)
        error=check_case(ops,result)
        if error:
            print(f'FAIL {name}: {error}')
            lines=result.stdout.splitlines()
            if lines: print('  Log final:', ' | '.join(lines[-3:]))
            if result.stderr.strip(): print('  stderr:',result.stderr.strip()[:350])
        else:
            passed+=1
            print(f'PASS {name}')
    try: binary.unlink()
    except OSError: pass
    print(f'\nRESULTADO FLAGS ESPECIAIS + DAA: {passed}/{len(cases)} testes passaram')
    return 0 if passed==len(cases) else 1

# Modelo separado da implementacao DAA em C: os testes BCD partem de
# operandos decimais validos e comparam o valor BCD matematicamente esperado.
def check_case(ops,result):
    if result.returncode != 0:
        return f'emulador retornou codigo {result.returncode}'
    traces=TRACE.findall(result.stdout)
    if len(traces)!=10:
        return f'{len(traces)} entradas de trace, esperado 10 (verifique formato do debugger)'
    pc=0x100; a=0x01; f=0xB0; cycles=0
    decimal_a=None
    for i, ((opcode,imm), found) in enumerate(zip(ops,traces),1):
        opcode_pc=pc
        pc += 1+len(imm)
        cycles += 1+len(imm)
        label=None
        if opcode==0x3E: # LD A,n8
            label='LD'; a=imm[0]; decimal_a=(a>>4)*10+(a&15)
        elif opcode==0xFE: # CP n8
            label='CP'; value=imm[0]; result8=(a-value)&255
            f=(0x80 if result8==0 else 0)|0x40|(0x20 if (a&15)<(value&15) else 0)|(0x10 if a<value else 0)
        elif opcode==0x37: # SCF
            label='SCF'; f=(f&0x80)|0x10
        elif opcode==0x3F: # CCF
            label='CCF'; f=(f&0x80)|(0 if f&0x10 else 0x10)
        elif opcode==0x2F: # CPL
            label='CPL'; a ^= 0xFF; f |= 0x60
        elif opcode in (0xC6,0xCE,0xD6,0xDE):
            label={0xC6:'ADD',0xCE:'ADC',0xD6:'SUB',0xDE:'SBC'}[opcode]
            v=imm[0]; carry=1 if (f&0x10 and opcode in (0xCE,0xDE)) else 0
            if opcode in (0xC6,0xCE):
                summ=a+v+carry; result8=summ&255
                f=(0x80 if result8==0 else 0)|(0x20 if (a&15)+(v&15)+carry>15 else 0)|(0x10 if summ>255 else 0)
                decimal_expected=decimal_a + ((v>>4)*10+(v&15)) + carry
            else:
                minuend=a-v-carry; result8=minuend&255
                f=(0x80 if result8==0 else 0)|0x40|(0x20 if (a&15)<((v&15)+carry) else 0)|(0x10 if a<v+carry else 0)
                decimal_expected=decimal_a - ((v>>4)*10+(v&15)) - carry
            a=result8
        elif opcode==0x27: # DAA
            label='DAA'
            if decimal_expected is None:
                return 'erro na definicao do teste: DAA sem operacao decimal anterior'
            a=bcd(decimal_expected%100)
            f=(0x80 if a==0 else 0)|(f&0x40)|(0x10 if decimal_expected<0 or decimal_expected>99 else 0)
        elif opcode==0x00:
            label='NOP'
        else:
            return f'opcode desconhecido no runner: {opcode:02X}'
        pc_actual,mnemonic,pc_after,aa,bb,cc,ff=found
        if int(pc_actual,16)!=opcode_pc or mnemonic!=label:
            return f'passo {i}: esperado {opcode_pc:04X}:{label}; veio {pc_actual}:{mnemonic}'
        actual=(int(pc_after,16),int(aa,16),int(bb,16),int(cc,16),int(ff,16))
        expected=(pc,a,0,0x13,f)
        if actual!=expected:
            fields=['PC','A','B','C','F']
            diffs=[f'{key}={val:04X} esperado {exp:04X}' if key=='PC' else f'{key}={val:02X} esperado {exp:02X}' for key,val,exp in zip(fields,actual,expected) if val!=exp]
            return f'passo {i} ({label}): '+', '.join(diffs)
    ic=COUNT.search(result.stdout); cyc=CYCLES.search(result.stdout)
    if not ic or int(ic.group(1))!=10: return 'quantidade de instrucoes nao e 10'
    if not cyc or int(cyc.group(1))!=cycles:
        return f'M-cycles={cyc.group(1) if cyc else "ausente"}; esperado {cycles}'
    return None

def make_cases():
    cases=[]
    # CP prepara combinacoes representativas de Z N H C; cada instrucao
    # especial preserva/limpa/muda flags de modo distinto.
    for instr,name in ((0x2F,'cpl'),(0x37,'scf'),(0x3F,'ccf')):
        for a,comp in ((0x00,0x00),(0x00,0x01),(0x10,0x01),(0x21,0x10),
                       (0x00,0x10),(0xFF,0x00),(0x0F,0x10),(0x80,0x80)):
            ops=[(0x3E,[a]),(0xFE,[comp]),(instr,[])]
            ops += [(0x00,[])]*(10-len(ops))
            cases.append((f'{name}_a{a:02x}_cp{comp:02x}',ops))
    # 4 familias de operacoes x 2 carry de entrada x 12 combinacoes validas BCD.
    values=((0,0),(9,1),(19,1),(39,61),(99,1),(99,99),
            (50,50),(10,1),(0,1),(80,20),(45,55),(90,9))
    for opname,opcode in (('add',0xC6),('adc',0xCE),('sub',0xD6),('sbc',0xDE)):
        for carry in (0,1):
            for x,y in values:
                ops=[(0x3E,[bcd(x)])]
                # Force carry explicitly; initial F=B0 has C=1.
                ops.append((0x37,[]))
                if not carry: ops.append((0x3F,[]))
                ops.extend(((opcode,[bcd(y)]),(0x27,[])))
                ops += [(0x00,[])]*(10-len(ops))
                cases.append((f'daa_{opname}_a{x:02d}_b{y:02d}_c{carry}',ops))
    return cases

if __name__=='__main__':
    try: sys.exit(runner())
    except Exception as exc:
        print('ERRO NO RUNNER:',exc,file=sys.stderr)
        sys.exit(2)
