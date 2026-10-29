#!/usr/bin/env python3
"""SM83 LD remaining opcodes: 7 opcodes, 11 scenarios, 10 steps each."""
from pathlib import Path
import re
import subprocess
import sys

ROOT = next((p for p in Path(__file__).resolve().parents if (p/'src').is_dir() and (p/'include').is_dir()), None)
if ROOT is None:
    sys.exit('ERRO: coloque esta pasta dentro de gb-dmg01/tests/ ou gb-dmg01/.')
OUT = ROOT / 'gameboy_ld_test'
SUITE = Path(__file__).resolve().parent

# Each tuple: (instruction bytes, expected PC after execution, cycle count, optionally register assertions after this instruction).
# CPU postboot: PC=0100 SP=FFFE AF=01B0 BC=0013 DE=00D8 HL=014D
# Opcodes included: 08, EA, FA, E2, F2, F9, F8.
CASES = [
 ('98_ld_a16_a', [
  ('3E 5A',2,{}), ('EA 00 C0',4,{}), ('3E 00',2,{'a':0}), ('FA 00 C0',4,{'a':0x5A})]),
 ('99_ld_a_a16', [
  ('21 10 C0',3,{}), ('36 A6',3,{}), ('FA 10 C0',4,{'a':0xA6})]),
 ('100_ld_a16_sp', [
  ('31 EF BE',3,{}), ('08 20 C0',5,{}), ('21 20 C0',3,{}), ('2A',2,{'a':0xEF}), ('46',2,{'b':0xBE})]),
 ('101_ld_c_a', [
  ('0E 80',2,{'c':0x80}), ('3E 67',2,{'a':0x67}), ('E2',2,{}), ('3E 00',2,{'a':0}), ('F2',2,{'a':0x67})]),
 ('102_ld_a_c', [
  ('21 90 FF',3,{}), ('36 9C',3,{}), ('0E 90',2,{'c':0x90}), ('F2',2,{'a':0x9C})]),
 ('103_ld_sp_hl', [
  ('21 10 D0',3,{}), ('F9',2,{}), ('01 34 12',3,{}), ('C5',4,{}), ('D1',3,{}), ('7A',1,{'a':0x12}), ('43',1,{'b':0x34})]),
 ('104_ld_hl_sp_positive_carry', [
  ('31 F8 C0',3,{}), ('F8 10',3,{'f':0x10}), ('7C',1,{'a':0xC1}), ('45',1,{'b':0x08})]),
 ('105_ld_hl_sp_negative', [
  ('31 08 C0',3,{}), ('F8 F7',3,{'f':0x00}), ('7C',1,{'a':0xBF}), ('45',1,{'b':0xFF})]),
 ('106_ld_hl_sp_half_carry', [
  ('31 0F C0',3,{}), ('F8 01',3,{'f':0x20}), ('7C',1,{'a':0xC0}), ('45',1,{'b':0x10})]),
 ('107_ld_hl_sp_both_carries', [
  ('31 FF C0',3,{}), ('F8 01',3,{'f':0x30}), ('7C',1,{'a':0xC1}), ('45',1,{'b':0x00})]),
 ('108_ld_c_a_boundary', [
  ('0E FF',2,{'c':0xFF}), ('3E 53',2,{'a':0x53}), ('E2',2,{}), ('3E 00',2,{'a':0}), ('F2',2,{'a':0x53})]),
]
OP_TO_NAME = {0x3E:'LD',0xEA:'LD',0xFA:'LD',0x21:'LD',0x36:'LD',0x31:'LD',0x08:'LD',0x2A:'LD',0x46:'LD',0x0E:'LD',0xE2:'LD',0xF2:'LD',0xF9:'LD',0x01:'LD',0xC5:'PUSH',0xD1:'POP',0x7A:'LD',0x43:'LD',0xF8:'LD',0x7C:'LD',0x45:'LD',0x00:'NOP'}
TRACE = re.compile(r'^([0-9a-fA-F]{4}):\s*(\w+).*\nAFTER\s*\|\s*PC:([0-9a-fA-F]{4})\s+A:([0-9a-fA-F]{2})\s+B:([0-9a-fA-F]{2})\s+C:([0-9a-fA-F]{2})\s+F:([0-9a-fA-F]{2})', re.M)
COUNTS = re.compile(r'Instructions:\s*(\d+).*?M-cycles:\s*(\d+)', re.S)

def main():
 print('Raiz do projeto:', ROOT)
 print('Compilando...', end=' ', flush=True)
 build = subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror=return-type','-Iinclude',*[str(p) for p in sorted((ROOT/'src').glob('*.c'))],'-o',str(OUT)],cwd=ROOT,capture_output=True,text=True)
 if build.returncode:
  print('FALHOU\n',build.stderr)
  return 1
 print('OK')
 passed = 0
 for name,program in CASES:
  rom = bytearray(32768)
  expected = []
  pc = 0x100
  cycles = 0
  for raw,mc,assertions in program:
   ins = bytes.fromhex(raw)
   rom[pc:pc+len(ins)] = ins
   expected.append((pc,pc+len(ins),OP_TO_NAME[ins[0]],assertions))
   pc += len(ins)
   cycles += mc
  while len(expected)<10:
   rom[pc] = 0
   expected.append((pc,pc+1,'NOP',{}))
   pc+=1
   cycles+=1
  path = SUITE / (name+'.gb')
  path.write_bytes(rom)
  run = subprocess.run([str(OUT),str(path)],cwd=ROOT,capture_output=True,text=True)
  output = run.stdout+'\n'+run.stderr
  if run.returncode:
   print('FAIL',path.name,'programa saiu com',run.returncode,'\n',output[-500:]);continue
  traces = list(TRACE.finditer(output))
  errors = []
  if len(traces)!=10:
   errors.append(f'Esperava 10 registros de trace, vieram {len(traces)}')
  for i, (start,nextpc,mnemonic,assertions) in enumerate(expected[:len(traces)]):
   match=traces[i]
   seen_start=int(match[1],16); seen_name=match[2].upper(); seen_pc=int(match[3],16)
   if seen_start!=start or seen_pc!=nextpc:
    errors.append(f'passo {i+1}: PC {seen_start:04X}->{seen_pc:04X}, esperado {start:04X}->{nextpc:04X}')
   if seen_name!=mnemonic:
    errors.append(f'passo {i+1}: instrução {seen_name}, esperado {mnemonic}')
   for r,v in assertions.items():
    field={'a':4,'b':5,'c':6,'f':7}[r]
    observed=int(match[field],16)
    if observed!=v:
     errors.append(f'passo {i+1}: {r.upper()}={observed:02X}, esperado {v:02X}')
  summary=COUNTS.search(output)
  if not summary:
   errors.append('Não encontrou contagem de instruções/ciclos')
  else:
   ins,mc=map(int, summary.groups())
   if ins!=10 or mc!=cycles:
    errors.append(f'Instructions={ins} M-cycles={mc}; esperado 10 e {cycles}')
  if errors:
   print('FAIL',path.name,':','; '.join(errors[:5]));print('  Log final:', ' | '.join(output.strip().splitlines()[-3:]))
  else:
   print(f'PASS {path.name:<37} 10 instruções, {cycles} M-cycles')
   passed+=1
 print(f'\nRESULTADO LD FINAL: {passed}/{len(CASES)} testes passaram')
 return 0 if passed==len(CASES) else 1

if __name__ == '__main__':sys.exit(main())
