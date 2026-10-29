# SM83 — LD Final Suite

11 cenários para os 7 opcodes recém-registrados: `EA FA 08 E2 F2 F9 F8`.

## Instalação

Extraia `ld_remaining_suite` para `gb-dmg01/tests/`.

Execute na raiz do projeto:

```bash
python3 tests/ld_remaining_suite/test_ld_remaining.py
```

Compila `src/*.c`, gera 11 ROMs de 32 KiB, executa o emulador em modo 10 instruções e valida cada PC/opcode, registradores observáveis, flags específicas e M-cycles totais.

**Importante:** testes `E2/F2` usam a high RAM nos endereços `FF80`, `FF90`, `FFFF`. Se o Bus ainda não implementa HRAM/IE, essas ROMs falharão mesmo que o fetch esteja certo. `FFFF` é o registrador IE e sua implementação exige atenção. O teste de fronteira 108 pode ser adiado até o IE estar mapeado.

O debugger atual não imprime SP, HL ou bytes da pilha; eles são verificados indiretamente por leituras e transferências. Isso não substitui testes diretos no Bus.
