# Testes de rotações SM83 — RLCA, RRCA, RLA e RRA

48 ROMs de 32 KiB, numeradas de `109` a `156`, 12 para cada opcode. Exercitam A inicial em 00, 01, 80, 81, 7F e FF, com Carry inicial tanto 0 quanto 1.

## Instalação

Extraia `rotation_suite` para `gb-dmg01/tests/rotation_suite/`.

Na raiz do `gb-dmg01`, execute:

```sh
python3 tests/rotation_suite/test_rotations.py
```

O runner procura automaticamente `src/` e `include/`, compila usando clang, **regenera todas as ROMs** e valida cada passo (PC, A, B, C, F), mnemônico da rotação, número de instruções (10) e M-cycles (13).

Cada ROM executa `LD A,valor`, `CP n8` para preparar Carry, `LD A,alvo` preservando as flags, a rotação e 6 `NOP`s. `CP` foi escolhido porque `SCF` ainda não está implementado.

Resultado esperado: `RESULTADO ROTAÇÕES: 48/48 testes passaram`.

**Observação:** as ROMs se apoiam em LD A,n8, CP n8 e NOP, já usados em testes anteriores. O runner não inspeciona SP nem memória; não são relevantes para estas quatro rotações.
