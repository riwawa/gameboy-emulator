# Testes CALL/RET condicionais (SM83)

Coloque esta pasta em `gb-dmg01/tests/call_ret_cond_suite/` e execute na raiz:

```sh
python3 tests/call_ret_cond_suite/test_call_ret_cond.py
```

16 ROMs (61 a 76): CALL NZ/Z/NC/C e RET NZ/Z/NC/C, com condicao verdadeira e falsa.

Cada ROM comeca em 0x0100 e o emulador executa 10 instrucoes.

**CALL:** condicao verdadeira chama 0x0110, executa `LD C,0x55`, `RET` e retorna para 0x0107; falsa segue 0x0107, sem executar a subrotina.

**RET:** `CALL 0x0110` incondicional entra na subrotina. `RET cc` verdadeiro retorna direto para 0x0107; falso continua para `LD C,0x55` e um `RET` incondicional.

O runner valida a sequencia de 10 PCs antes/depois, A/B/C/F, instrucoes e M-cycles totais. Como o debugger nao mostra SP, os testes nao verificam diretamente o valor do SP nem os bytes da pilha; o retorno correto verifica indiretamente o endereco empilhado.

Ciclos: CALL cc 6/3; RET cc 5/2 (realizado/nao realizado).
