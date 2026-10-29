#!/usr/bin/env python3
"""Auditoria estrutural da tabela de opcodes SM83 (nao substitui testes funcionais)."""
from pathlib import Path
from collections import Counter
import csv
import re
import subprocess
import sys
import tempfile

ILLEGAL = {0xD3,0xDB,0xDD,0xE3,0xE4,0xEB,0xEC,0xED,0xF4,0xFC,0xFD}


def root_from(here):
    for p in [here, *here.parents]:
        if (p/'src/instructions.c').is_file() and (p/'include/instructions.h').is_file():
            return p
    raise RuntimeError('Nao encontrei src/instructions.c e include/instructions.h; coloque a suite dentro do gb-dmg01')


def enum_members(source, enum_name):
    # Este enum e simples (constantes IN_* sem expressoes). Gera mapeamento simbolico
    # para o harness C, eliminando suposicoes sobre os numeros dos enumeradores.
    m = re.search(r'typedef\s+enum\s*\{([^{}]*)\}\s*'+re.escape(enum_name)+r'\s*;', source, re.S)
    if not m:
        raise RuntimeError(f'Enum {enum_name} nao encontrado em instructions.h')
    body = re.sub(r'/\*.*?\*/|//[^\n]*', '', m.group(1), flags=re.S)
    return re.findall(r'\bIN_[A-Z0-9_]+\b', body)


def run_audit(root):
    src = (root/'include/instructions.h').read_text(encoding='utf-8')
    symbols = enum_members(src, 'in_type')
    if len(set(symbols)) != len(symbols):
        raise RuntimeError('O enum in_type tem simbolos repetidos')
    sym_cases = '\n'.join(f'case {name}: return "{name}";' for name in symbols)
    harness = f'''#include <stdio.h>
#include "instructions.h"
static const char *typename_(in_type t) {{
    switch (t) {{
{sym_cases}
        default: return "IN_UNKNOWN_ENUM";
    }}
}}
int main(void) {{
    instructions_init();
    for (int op = 0; op < 256; ++op) {{
        instruction *i = instruction_by_opcode((unsigned char)op);
        if (!i) printf("%02X|UNREGISTERED|-1\\n", op);
        else printf("%02X|%s|%d\\n", op, typename_(i->type), (int)i->mode);
    }}
    return 0;
}}
'''
    with tempfile.TemporaryDirectory(prefix='sm83audit_') as t:
        td = Path(t)
        c = td/'audit.c'
        exe = td/'audit'
        c.write_text(harness)
        cc = subprocess.run(['clang','-std=c11','-Wall','-Wextra','-I'+str(root/'include'), str(root/'src/instructions.c'),str(c),'-o',str(exe)],capture_output=True,text=True)
        if cc.returncode:
            print(cc.stderr)
            raise RuntimeError('Falha ao compilar harness de registro')
        out = subprocess.run([str(exe)],capture_output=True,text=True,check=True).stdout
    rows = {}
    for line in out.splitlines():
        op, typ, mode = line.split('|')
        rows[int(op, 16)] = (typ, int(mode))
    if len(rows)!=256:
        raise RuntimeError(f'Harness retornou {len(rows)} linhas em vez de 256')

    proc = (root/'src/cpu_proc.c').read_text(encoding='utf-8') if (root/'src/cpu_proc.c').is_file() else ''
    all_src = '\n'.join(p.read_text(encoding='utf-8',errors='replace') for p in (root/'src').glob('*.c'))
    dispatch = set(re.findall(r'\bcase\s+(IN_[A-Z0-9_]+)\s*:', proc))
    modes = set(re.findall(r'\bcase\s+(AM_[A-Z0-9_]+)\s*:', all_src))
    # Mapa do enum para checar se ha uma branch de fetch para cada modo.
    mode_enum = re.search(r'typedef\s+enum\s*\{([^{}]*)\}\s*addr_mode\s*;', src,re.S)
    mode_names = re.findall(r'\bAM_[A-Z0-9_]+\b', re.sub(r'/\*.*?\*/|//[^\n]*','',mode_enum.group(1),flags=re.S)) if mode_enum else []
    by_mode = {i:n for i,n in enumerate(mode_names)}

    missing = [i for i in range(256) if i not in ILLEGAL and rows[i][0]=='UNREGISTERED']
    wrong = [i for i in ILLEGAL if rows[i][0]!='UNREGISTERED']
    main_registered = [i for i in range(256) if i not in ILLEGAL and rows[i][0]!='UNREGISTERED']
    no_dispatch = sorted({rows[i][0] for i in main_registered if rows[i][0] not in dispatch})
    no_mode = sorted({(i,by_mode.get(rows[i][1],f'MODE_{rows[i][1]}')) for i in main_registered if by_mode.get(rows[i][1]) not in modes})

    print('AUDITORIA DE OPCODES SM83 — registro e estrutura')
    print('Raiz:',root)
    print('Opcodes primarios validos: 245 (inclui prefixo CB)')
    print('Opcodes primarios registrados: %d/245' % len(main_registered))
    print('Opcodes primarios validos ausentes: %d' % len(missing))
    print('Opcodes ilegais indevidamente registrados: %d' % len(wrong))
    print('Tipos sem case IN_* no cpu_proc.c: %d' % len(no_dispatch))
    print('Registrados com modo sem case AM_* em src/: %d' % len(no_mode))
    print('Prefixo CB: %s' % ('registrado' if rows[0xCB][0]=='IN_CB' else 'NAO registrado como IN_CB'))
    print('Combinacoes CB de segundo byte: 256 (verificadas funcionalmente na suite CB separada)')
    print('Total de codificacoes validas: 500 (244 nao-CB + 256 CB)')
    if missing: print('\nFALTANDO:', ', '.join(f'{i:02X}' for i in missing))
    if wrong: print('\nILEGAIS REGISTRADOS:', ', '.join(f'{i:02X}' for i in sorted(wrong)))
    if no_dispatch: print('\nSEM DISPATCH:', ', '.join(no_dispatch))
    if no_mode: print('\nMODOS SEM BRANCH:', ', '.join(f'{op:02X}={mode}' for op,mode in no_mode))

    path = root/'tests'/'opcode_audit_suite'/'opcode_audit.csv'
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open('w',newline='',encoding='utf-8') as f:
        writer=csv.writer(f)
        writer.writerow(['opcode_hex','legal','type','mode','dispatch','fetch_mode_branch'])
        for op,(typ, mode) in sorted(rows.items()):
            writer.writerow([f'{op:02X}',int(op not in ILLEGAL),typ,by_mode.get(mode,str(mode)),int(typ in dispatch),int(by_mode.get(mode) in modes)])
    print('\nRelatorio CSV:',path.relative_to(root))
    if not (missing or wrong or no_dispatch or no_mode) and rows[0xCB][0]=='IN_CB':
        print('\nRESULTADO ESTRUTURAL: PASS — registro/dispatch/fetch sem lacunas detectadas')
        print('ATENCAO: nao prova execucao, ciclos, interrupcoes ou comportamento de STOP.')
        return 0
    print('\nRESULTADO ESTRUTURAL: PENDENCIAS — veja os itens acima')
    return 1


if __name__=='__main__':
    try:
        sys.exit(run_audit(root_from(Path(__file__).resolve().parent)))
    except (RuntimeError, OSError, subprocess.CalledProcessError) as e:
        print('ERRO:', e, file=sys.stderr)
        sys.exit(2)
