#!/usr/bin/env python3
"""Разрешение git-конфликтов в деревьях Solstice / 26.62.

Режимы:
  report                — показать все блоки (файл, строка, размеры сторон)
  resolve <tree> <rule> — записать разрешённые файлы
        rule: head | other | union   (union = HEAD, затем уникальные строки OTHER)

После resolve печатается отчёт о дубликатах определений функций.
"""
import argparse
import os
import re
import sys
from collections import defaultdict

MARK_OPEN = re.compile(r'^<{7,}', re.M)
MARK_MID = re.compile(r'^={7,}')
MARK_CLOSE = re.compile(r'^>{7,}')

EXT = ('.cpp', '.hpp', '.h', '.cxx', '.cc', '.inl')


def iter_source_files(root):
    for base, _dirs, files in os.walk(root):
        if '/.git' in base or '/out' in base or '/build' in base:
            continue
        for f in files:
            if f.endswith(EXT) or f == 'CMakeLists.txt':
                yield os.path.join(base, f)


def parse_conflicts(lines):
    """Возвращает список (start, head_lines, other_lines, end_index)."""
    out = []
    i = 0
    while i < len(lines):
        if MARK_OPEN.match(lines[i]):
            start = i
            i += 1
            head = []
            while i < len(lines) and not MARK_MID.match(lines[i]):
                head.append(lines[i])
                i += 1
            i += 1
            other = []
            while i < len(lines) and not MARK_CLOSE.match(lines[i]):
                other.append(lines[i])
                i += 1
            out.append((start, head, other, i))
        i += 1
    return out


def resolve_lines(lines, rule):
    conflicts = parse_conflicts(lines)
    if not conflicts:
        return lines, 0
    result = []
    pos = 0
    for start, head, other, end in conflicts:
        result.extend(lines[pos:start])
        if rule == 'head':
            chosen = head
        elif rule == 'other':
            chosen = other
        else:  # union
            chosen = list(head)
            seen = {l.strip() for l in head if l.strip()}
            for line in other:
                if line.strip() and line.strip() in seen:
                    continue
                chosen.append(line)
        result.extend(chosen)
        pos = end + 1
    result.extend(lines[pos:])
    return result, len(conflicts)


DEF_RE = re.compile(
    r'^[A-Za-z_][\w:<>,&\* \t]*?([A-Za-z_]\w*)::([A-Za-z_]\w*)\s*\(')


def find_duplicate_definitions(lines):
    """Грубая проверка: повторяющиеся определения Class::method."""
    seen = defaultdict(list)
    for n, line in enumerate(lines, 1):
        m = DEF_RE.match(line)
        if m and not line.lstrip().startswith('//'):
            seen[f'{m.group(1)}::{m.group(2)}'].append(n)
    return {k: v for k, v in seen.items() if len(v) > 1}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=['report', 'resolve'])
    ap.add_argument('tree', nargs='?', default='src')
    ap.add_argument('rule', nargs='?', default='head',
                    choices=['head', 'other', 'union'])
    ap.add_argument('--write', action='store_true',
                    help='реально писать файлы (по умолчанию dry-run)')
    args = ap.parse_args()

    total_blocks = 0
    total_files = 0
    dup_report = []

    for path in sorted(iter_source_files(args.tree)):
        with open(path, encoding='utf-8', errors='ignore') as fh:
            text = fh.read()
        if not MARK_OPEN.search(text):
            continue
        lines = text.split('\n')
        conflicts = parse_conflicts(lines)
        total_files += 1
        total_blocks += len(conflicts)

        if args.mode == 'report':
            print(f'\n{path}: {len(conflicts)} блок(ов)')
            for idx, (start, head, other, _end) in enumerate(conflicts, 1):
                print(f'  #{idx:<2} стр.{start + 1:<6} HEAD={len(head):<4} '
                      f'OTHER={len(other):<4}')
            continue

        resolved, _n = resolve_lines(lines, args.rule)
        dup = find_duplicate_definitions(resolved)
        if dup:
            dup_report.append((path, dup))
        if args.write:
            with open(path, 'w', encoding='utf-8', newline='\n') as fh:
                fh.write('\n'.join(resolved))

    if args.mode == 'resolve':
        verb = 'записано' if args.write else 'DRY-RUN (ничего не изменено)'
        print(f'{verb}: {total_files} файл(ов), {total_blocks} блок(ов), '
              f'правило "{args.rule}"')
        if dup_report:
            print('\nВНИМАНИЕ: повторяющиеся определения функций '
                  '(нужен ручной разбор):')
            for path, dup in dup_report:
                print(f'  {path}')
                for name, where in dup.items():
                    print(f'    {name}: строки {", ".join(map(str, where))}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
