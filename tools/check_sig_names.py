#!/usr/bin/env python3
"""
Проверяет, что каждое имя, к которому обращаются как `SigManager::X` или
`OffsetProvider::X`, реально объявлено в соответствующем заголовке.

Почему это важно: если имени нет (его забыли при переносе, а сигнатуру не
сняли), компилятор скажет «нет такого члена» — и весь клиент не соберётся.
Так было с `SigManager::ItemRenderer_getIconUV` (иконки предметов) и
`OffsetProvider::MinecraftGame_playUi` (звуки UI).

«Объявлено» = любое из:
  * DEFINE_SIG(name, ...) / DEFINE_FIELD(name, ...) без комментария;
  * static inline uintptr_t name = 0;  — осознанная заглушка «не сканировали».

Usage:
    python3 tools/check_sig_names.py [26.62/src]
Код возврата: 0 — все имена объявлены, 1 — есть висячие ссылки.
"""

import os
import re
import sys

ROOT = '26.62/src'
SIG_HDR = 'SDK/SigManager.hpp'
OFF_HDR = 'SDK/OffsetProvider.hpp'

OWN = {'initialize', 'deinitialize', 'scanSig', 'mSigs', 'SigManager', 'OffsetProvider'}


def strip_comments(text):
    return re.sub(r'//[^\n]*', '', text)


# у каждого макроса имя стоит в своей позиции в списке аргументов
NAME_ARG = {
    'DEFINE_SIG': 0, 'DEFINE_FIELD': 0,
    'DEFINE_INDEX_FIELD': 0, 'DEFINE_INDEX_FIELD_TYPED': 1,
}


def declared_names(path):
    raw = open(path, encoding='utf-8', errors='ignore').read()
    live = strip_comments(raw)
    names = set()
    for macro, pos in NAME_ARG.items():
        for args in re.findall(macro + r'\(([^)]*)\)', live):
            parts = [a.strip() for a in args.split(',')]
            if len(parts) > pos and re.fullmatch(r'\w+', parts[pos]):
                names.add(parts[pos])
    names |= set(re.findall(r'static inline uintptr_t\s+(\w+)\s*=\s*0', raw))
    return names


def used_names(root, prefix):
    used = set()
    for dirpath, _, files in os.walk(root):
        for f in files:
            if not f.endswith(('.cpp', '.hpp')):
                continue
            t = strip_comments(open(os.path.join(dirpath, f), encoding='utf-8', errors='ignore').read())
            used |= set(re.findall(prefix + r'::(\w+)', t))
    return used


def main():
    root = ROOT
    if len(sys.argv) > 1:
        root = sys.argv[-1]

    problems = []
    for prefix, rel in (('SigManager', SIG_HDR), ('OffsetProvider', OFF_HDR)):
        hdr = os.path.join(root, rel)
        have = declared_names(hdr)
        missing = sorted(used_names(root, prefix) - have - OWN)
        for m in missing:
            problems.append(f'{prefix}::{m} — нет объявления в {hdr}')

    for p in problems:
        print(p)
    print(f'\n{len(problems)} висячих ссылок на SigManager/OffsetProvider')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
