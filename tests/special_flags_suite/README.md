# CPU SM83 — CPL, SCF, CCF e DAA

Suíte de 120 ROMs: 24 testes de flags especiais e 96 testes de DAA após ADD, ADC, SUB e SBC com operandos BCD válidos. Avalia cada passo (PC, A, B, C, F), instruções totais e M-cycles.

## Uso

Extraia `special_flags_suite/` em `gb-dmg01/tests/` e execute na raiz do projeto:

```sh
python3 tests/special_flags_suite/test_special_flags.py
```

O runner compila `src/*.c` com Clang, gera 120 ROMs `.gb` de 32 KiB na própria pasta, executa todas, relata falhas e retorna status 1 se houver erro. `main.c` deve aceitar o caminho da ROM e executar 10 instruções por execução, emitindo linhas `0100: LD ...` / `AFTER | PC:... A:... B:... C:... F:...`, além de `Instructions: 10` e `M-cycles: ...`.

Os valores BCD são obtidos por aritmética decimal independente da lógica usada por `proc_daa` em C. Não cobre todos os 256 x 256 operandos possíveis, nem DAA após entradas BCD inválidas.
