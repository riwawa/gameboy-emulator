# Testes CALL/RET — Game Boy SM83

Coloque `ret_call_suite/` dentro de `gb-dmg01/` ou `gb-dmg01/tests/`.
O script encontra `src/` e `include/` subindo diretórios.

Execute na raiz do projeto:

```sh
python3 tests/ret_call_suite/test_ret_call.py
```

Ou se moveu a pasta para a raiz:

```sh
python3 ret_call_suite/test_ret_call.py
```

A suíte recompila `src/*.c` antes de cada execução do conjunto.

## Pré-requisitos

- `CALL a16` (opcode `CD`): busca destino little-endian, empilha PC de retorno e salta; 6 M-cycles.
- `RET` (opcode `C9`): desempilha PC; 4 M-cycles.
- `INC B`, `INC C`, `NOP` e memória da pilha (`0xFFFC` e `0xFFFD`).
- CPU iniciada com BC=`0013`, PC=`0100` e SP=`FFFE` (inicialização do projeto).
- Loop de execução limitado a **10 instruções** com linhas `AFTER | PC:... A:... B:... C:... F:...` e contadores finais.

## Casos

- `25_call_ret_basic.gb`: CALL -> INC C -> RET -> INC B, verifica endereço de retorno.
- `26_call_ret_twice.gb`: duas chamadas consecutivas, retornando corretamente de ambas.
- `27_call_ret_nested.gb`: chamadas aninhadas, garantindo ordem LIFO da pilha.
- `28_call_ret_high_address.gb`: chamada para `0x0234` e retorno, valida endereço absoluto com byte alto != 0.

Verifica cada PC/B/C/F **após toda instrução** e os M-cycles totais. Não verifica diretamente SP nem conteúdo da pilha; se quiser isso, acrescente SP ao log da CPU ou crie acesso programático ao estado interno.

Se uma ROM falhar, o runner mostra o primeiro passo divergente. Um FAIL pode indicar `CALL`, `RET`, leitura/escrita do Bus ou ciclos; não presume a causa.
