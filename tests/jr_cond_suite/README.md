# JR condicional — SM83

Oito ROMs (45–52), duas por condicao NZ, Z, NC e C. Cada ROM executa LD A,n8; CP n8; JR cond,+2; um LD B,EE que e pulado quando o salto acontece; INC B; NOPs ate 10 instrucoes.

Execucao, da raiz do projeto:

```bash
python3 tests/jr_cond_suite/test_jr_cond.py
```

A pasta jr_cond_suite tambem pode ficar diretamente na raiz de gb-dmg01. O runner encontra src/ automaticamente.

O runner confere o PC imediatamente apos o JR, F preservado pelo JR, registrador B e PC finais, quantidade de instrucoes e ciclos (14 M-cycles por ROM). O valor de F final muda porque INC B atualiza flags.
