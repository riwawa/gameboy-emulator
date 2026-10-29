# RST test suite — Game Boy SM83

8 ROMs para todos os restart vectors: 00, 08, 10, 18, 20, 28, 30 e 38.

## Instalação
Extraia `rst_suite` **em qualquer lugar dentro da raiz do repositório** `gb-dmg01`:
- `gb-dmg01/rst_suite/`, ou
- `gb-dmg01/tests/rst_suite/`.

## Execução
A partir de `gb-dmg01`:

```bash
python3 tests/rst_suite/test_rst.py
```

Se extraiu na raiz, use `python3 rst_suite/test_rst.py`.

Requer `clang` e Python 3. O runner localiza automaticamente `src/` e `include/`, recompila `gameboy` e regenera as ROMs de 32 KiB.

## O que cada ROM faz
- `0x0100`: `RST vector` (1 byte)
- `0x0101`: `INC B`
- `vector`: `INC C`
- `vector+1`: `RET`
- Restante da ROM: NOP (zeros)

São executadas 10 instruções, e o runner verifica **cada uma das quatro primeiras transições**, incluindo PCs de salto e retorno, os registradores A/B/C/F e o estado final.

Resultado esperado: `PC=0108`, `A=01`, `B=01`, `C=14`, `F=10`, `Instructions=10`, `M-cycles=16`.

## Limite
O runner valida indiretamente a pilha por meio do retorno correto ao PC=0101, mas **não lê diretamente SP ou bytes da RAM**. Para isso é preciso expor esses campos em logs ou numa API de teste.
