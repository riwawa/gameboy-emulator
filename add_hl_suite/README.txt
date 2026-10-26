8 ROMs de teste para ADD HL,rr (Game Boy SM83)

Instalacao:
  1. Descompacte a pasta add_hl_suite dentro de gb-dmg01/.
  2. Confira que tem gb-dmg01/src, gb-dmg01/include e gb-dmg01/add_hl_suite.
  3. Na raiz gb-dmg01 execute: python3 add_hl_suite/test_add_hl.py

O runner recompila todos os arquivos src/*.c, executa cada ROM (10 instrucoes),
confere A, B, F, PC e M-cycles, e apresenta PASS/FAIL.

Para testar HL sem exigir alteracao no seu printf, cada ROM copia H em A
(opcode 0x7C: LD A,H) e L em B (opcode 0x45: LD B,L) depois da soma.
Essas copias nao alteram flags. Logo, A e B finais representam HL final.

Dependencias:
  - Python 3 (biblioteca padrao apenas)
  - clang
  - Instrucao LD rr,n16; ADD HL,rr; LD A,H; LD B,L; OR A; NOP implementadas.
  - CPU inicia no estado DMG pos-boot com Z=1, A=01, F=B0.
  - Execucao limitada a 10 instrucoes.

Cenarios:
  16 ADD HL,BC: 1000 + 0020 = 1020 (Z preservado em 1)
  17 ADD HL,DE: 0FFF + 0001 = 1000 (H=1)
  18 ADD HL,HL: 8000 + 8000 = 0000 (C=1; Z ainda 1)
  19 ADD HL,SP: FFFF + 0001 = 0000 (H=1, C=1)
  20 ADD HL,BC: 0FFF + 0001 = 1000 com Z=0 (OR A primeiro)
  21 ADD HL,DE: FFFF + 0001 = 0000 (H=1,C=1)
  22 ADD HL,SP: 1234 + 0100 = 1334 (sem H/C)
  23 ADD HL,HL: 0800 + 0800 = 1000 (H=1)

Arquivos .gb sao ROMs artificiais de 32 KiB para o emulador customizado;
nao sao ROMs comerciais, nao contem Nintendo logo nem checksum valida.
