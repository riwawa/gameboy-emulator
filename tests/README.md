# Testes da ALU SM83

ROMs de 32 KiB, tipo ROM-only, programa em 0x0100.
Estado inicial esperado: DMG pós-boot (A=01, F=B0, PC=0100).
Cada programa contém instruções seguidas de NOP (00).
Para conferir ciclos exatos, execute apenas a quantidade de instruções indicada, alterando temporariamente `max_steps` no emulador.
As flags F são Z N H C nos bits 7..4.

| ROM | Instruções | A final | F final | PC final | M-cycles |
|---|---:|---|---|---|---:|
| `01_add_basic.gb` | 3 | `10` | `20` | `0105` | 5 |
| `02_adc_carry.gb` | 3 | `10` | `20` | `0105` | 5 |
| `03_sub_halfborrow.gb` | 3 | `0F` | `60` | `0105` | 5 |
| `04_sbc_carry.gb` | 3 | `0F` | `60` | `0105` | 5 |
| `05_and_basic.gb` | 3 | `03` | `20` | `0105` | 5 |
| `06_xor_zero.gb` | 3 | `00` | `80` | `0105` | 5 |
| `07_or_basic.gb` | 3 | `AF` | `00` | `0105` | 5 |
| `08_cp_equal.gb` | 3 | `05` | `C0` | `0105` | 5 |
| `09_add_overflow.gb` | 3 | `00` | `B0` | `0105` | 5 |
| `10_sbc_borrow.gb` | 3 | `00` | `F0` | `0105` | 5 |
| `11_cp_borrow.gb` | 3 | `02` | `70` | `0105` | 5 |
| `12_and_immediate.gb` | 2 | `00` | `A0` | `0104` | 4 |
| `13_xor_memory.gb` | 4 | `FF` | `00` | `0108` | 10 |
| `14_or_immediate.gb` | 2 | `AF` | `00` | `0104` | 4 |
| `15_cp_immediate.gb` | 2 | `05` | `C0` | `0104` | 4 |

## Executar

```sh
clang -std=c11 -Wall -Wextra -Iinclude src/*.c -o gameboy
./gameboy tests/01_add_basic.gb
```

Caso o trace imprima A antes da execução, procure o valor no início da instrução seguinte, ou adicione A ao `AFTER`.
A execução de 10 instruções produzirá NOPs extras: cada NOP soma 1 M-cycle e avança PC em 1.

## Bytes usados

- `01_add_basic`: 3E 0F 06 01 80
- `02_adc_carry`: 3E 0F 06 00 88
- `03_sub_halfborrow`: 3E 10 06 01 90
- `04_sbc_carry`: 3E 10 06 00 98
- `05_and_basic`: 3E 0F 06 03 A0
- `06_xor_zero`: 3E 55 06 55 A8
- `07_or_basic`: 3E A0 06 0F B0
- `08_cp_equal`: 3E 05 06 05 B8
- `09_add_overflow`: 3E FF 06 01 80
- `10_sbc_borrow`: 3E 00 06 FF 98
- `11_cp_borrow`: 3E 02 06 05 B8
- `12_and_immediate`: 3E 0F E6 F0
- `13_xor_memory`: 21 00 C0 36 F0 3E 0F AE
- `14_or_immediate`: 3E A0 F6 0F
- `15_cp_immediate`: 3E 05 FE 05
