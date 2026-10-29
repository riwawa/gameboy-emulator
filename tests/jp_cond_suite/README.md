# Testes de JP condicional (SM83)

Oito ROMs que cobrem JP NZ/Z/NC/C com saltos realizados e não realizados.

## Instalação

Extraia `jp_cond_suite` dentro de `gb-dmg01/tests/` (ou diretamente na raiz). O runner identifica a raiz automaticamente.

## Executar

```bash
python3 tests/jp_cond_suite/test_jp_cond.py
```

O runner recompila `src/*.c` com clang e valida opcode, endereço do JP (0x0104), PC imediatamente após a decisão, flags, registradores finais e ciclos. Não depende do mnemônico impresso pelo debugger.

## Programa comum

```asm
0100: LD A,x
0102: CP y
0104: JP cc,0110
0107: LD B,EE ; caminho sem salto
0109: INC B  ; B = EF
010A: JP 0120
0110: INC B  ; caminho com salto, B = 01
0111: JP 0120
0120: NOP...
```

Executa 10 instruções por ROM. O contador total é 18 M-cycles por caso (os caminhos têm instruções diferentes). A diferença 4 vs. 3 M-cycles da instrução JP é garantida pela especificação e pode ser verificada isoladamente em uma futura instrumentação do emulador.
