# CB-prefixed — 512 testes (256 opcodes × 2 variantes)

Coloque a pasta `cb_suite` dentro de `gb-dmg01/tests/` e rode da raiz do projeto:

```bash
python3 tests/cb_suite/test_cb.py
```

O script:
1. Encontra a raiz do projeto por `src/` e `include/`;
2. Compila o executável de teste com clang;
3. Gera 512 ROMs de 32 KiB diretamente na pasta;
4. Executa cada ROM por 10 instruções;
5. Verifica PC após CB, flags F, valor de A após uma instrução `LD A,r` / `LD A,(HL)`, contagem de instruções e ciclos;
6. Emite PASS/FAIL com o opcode.

O modelo de referência em Python calcula RLC/RRC/RL/RR/SLA/SRA/SWAP/SRL, BIT/RES/SET independentemente da função C do emulador. Usa valores e flags iniciais diferentes nas duas variantes de cada opcode. Para `(HL)`, escreve na WRAM em C000 e verifica a leitura posterior; para `BIT`, testa zero e um. `RES` e `SET` também testam condições em que o bit já estava no estado desejado.

**Limitação:** como o log atual não expõe os registradores D/E/H/L ou o barramento, a comparação nesses casos observa o valor carregado posteriormente em A; ela não inspeciona a memória diretamente. Isso ainda não substitui testes abrangentes de integração e comportamento de hardware.

A execução gera arquivos `.gb` e um executável temporário `_gameboy_cb_test` (removido ao final). As ROMs são intencionalmente curtas e não possuem cabeçalho de cartucho válido, pois o ambiente de testes usa ROMs cruas de 32 KiB.
