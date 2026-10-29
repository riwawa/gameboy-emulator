# Suite PUSH/POP — Game Boy SM83

Coloque esta pasta em `gb-dmg01/tests/push_pop_suite/` e, na raiz do projeto, execute:

```bash
python3 tests/push_pop_suite/test_push_pop.py
```

O runner recompila o emulador, gera 12 ROMs de 32 KiB (78–89) e executa 10 instruções em cada uma. Verifica opcode, PC antes/depois, A/B/C/F em cada instrução e contagem total de M-cycles. Cobre BC, DE, HL, AF, mascara F no POP AF, cruzamento de pares e ordem LIFO. Para D/E/H/L, testa leituras por `LD A,r8`.

**Limite de observabilidade:** o debugger atual não mostra SP ou RAM; logo, a suíte **não afirma verificar diretamente** SP/bytes na memória. Verifica-os indiretamente pelas restaurações e pelos endereços/ciclos. Para verificar diretamente SP e RAM, exponha esses valores no debugger ou crie testes unitários para stack_push16/stack_pop16.

Os testes partem do estado post-boot conhecido: PC 0100, SP FFFE, AF 01B0, BC 0013, DE 00D8, HL 014D. Pressupõem que o emulador executa 10 instruções por chamada e aceita ROM por argumento.
