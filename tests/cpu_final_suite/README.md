# CPU SM83 — suite de integração final (primeira rodada)

Coloque `cpu_final_suite/` em `gb-dmg01/tests/`, e execute na raiz:

```bash
python3 tests/cpu_final_suite/test_cpu_final.py
```

O runner compila `src/*.c`, gera ROMs `.gb` locais (32 KiB), executa o binário (modelo atual: `main` com 10 chamadas de `cpu_step`) e confere um subconjunto de transições de PC, A e F.

Verificações: ADD SP,e8 (positivo, negativo e zero), JP HL, atraso EI, DI cancelando EI, DI bloqueando interrupção, RETI, prioridade VBlank vs LCD, HALT bug, HALT ocioso e STOP básico.

Limites: o runner não consegue observar diretamente IME, SP ou IF; eles são inferidos indiretamente. ROMs com HALT/STOP usam execução de 10 passos fixos; uma implementação que pausa o main por completo pode requerer um harness diferente. Não cobre todas as prioridades, todos os vetores, nem temporização precisa de interrupções. Se algum teste falhar, envie a saída para distinguir bug na CPU de hipótese incorreta do runner.
