# LD indireto: suite SM83 (8 ROMs)

Valida os opcodes 02, 12, 0A, 1A, 22, 2A, 32, 3A, numerados 90–97.

Coloque `ld_indirect_suite` em `gb-dmg01/tests/` e execute na raiz do projeto:

```bash
python3 tests/ld_indirect_suite/test_ld_indirect.py
```

O script recompila o emulador, gera as ROMs de 32 KiB e executa dez instruções por ROM. Compara opcode, PC antes e depois, A/B/C/F a cada etapa e M-cycles finais com uma simulação de referência independente. Usa WRAM em `0xC100` e supõe o estado pós-boot do projeto (PC 0100, A 01, BC 0013, F B0). Não verifica diretamente SP, D/E, H/L nem conteúdo da memória; os testes de HL+/- inferem os endereços acessados pelos valores posteriormente lidos.

Se algum teste falhar, compartilhe a linha FAIL e o log do emulador.
