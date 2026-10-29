# Auditoria estrutural dos opcodes SM83

Coloque `opcode_audit_suite` dentro de `gb-dmg01/tests/` e execute da raiz:

```sh
python3 tests/opcode_audit_suite/audit_opcodes.py
```

O script compila `src/instructions.c` com um harness temporário, chama a função **real** `instructions_init()` e consulta `instruction_by_opcode()` para todos os 256 opcodes principais. Faz então checagens estruturais dos `case IN_*` em `src/cpu_proc.c` e dos `case AM_*` em `src/` e gera `tests/opcode_audit_suite/opcode_audit.csv`.

**O que esse teste comprova:** presença dos 245 opcodes principais válidos, ausência dos 11 opcodes ilegais, presença de entrada `IN_CB` e conexão aparente com dispatcher e modo de endereçamento.

**O que NÃO comprova:** que cada opcode funcione corretamente, que toda função seja alcançável, comportamento de interrupções, corretude dos 256 CB, memórias, e temporização. Os 256 CB devem ser verificados com a suíte CB anterior (512 cenários). Dos 245 primários válidos, um é o prefixo CB, então o total de codificações de instruções válidas é **500**, não 501.

Se um tipo estiver registrado com outra forma de dispatch (sem `case IN_*` em `cpu_proc.c`), o relatório pode acusá-lo como pendente mesmo que haja execução; avalie manualmente.

O script não altera o código C do projeto. Só gera o CSV de auditoria.
