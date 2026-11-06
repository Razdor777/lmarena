#!/usr/bin/env python3
"""Инструмент для разрешения merge-конфликтов в дереве 26.62.

Причина существования: база 26.62 пришла в репозиторий в состоянии
наполовину сделанного мержа (файлы содержат маркеры <<<<<<< / ======= / >>>>>>>).
Скрипт умеет показать конфликты и применить выбранную сторону.

Использование:
    python3 tools/resolve_conflicts.py list  <file...>
    python3 tools/resolve_conflicts.py ours  <file...>
    python3 tools/resolve_conflicts.py theirs <file...>
    python3 tools/resolve_conflicts.py check <file...>   # вернёт 1, если маркеры остались
"""
import re
import sys

START = re.compile(r'^<{7}')
MID = re.compile(r'^={7}$')
END = re.compile(r'^>{7}')


def parse(lines):
    """Возвращает список блоков и остальной текст как единый поток."""
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if START.match(line):
            start = i
            j = i + 1
            while j < len(lines) and not MID.match(lines[j].rstrip('\n')):
                j += 1
            k = j + 1
            while k < len(lines) and not END.match(lines[k]):
                k += 1
            if j >= len(lines) or k >= len(lines):
                raise SystemExit(f'незакрытый конфликт на строке {start + 1}')
            out.append({
                'kind': 'conflict',
                'line': start + 1,
                'label': line.strip(),
                'ours': lines[i + 1:j],
                'theirs': lines[j + 1:k],
            })
            i = k + 1
        else:
            out.append({'kind': 'text', 'text': line})
            i += 1
    return out


def count_markers(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        return sum(1 for line in f if START.match(line))


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    cmd, paths = sys.argv[1], sys.argv[2:]

    if cmd == 'check':
        bad = [p for p in paths if count_markers(p)]
        for p in bad:
            print(p, count_markers(p))
        return 1 if bad else 0

    for path in paths:
        with open(path, encoding='utf-8', errors='replace') as f:
            lines = f.readlines()
        blocks = parse(lines)

        if cmd == 'list':
            n = 0
            for b in blocks:
                if b['kind'] != 'conflict':
                    continue
                n += 1
                print(f'\n== {path}:{b["line"]}  [{b["label"]}]')
                print(f'   ours={len(b["ours"])} строк / theirs={len(b["theirs"])} строк')
                for t in b['ours'][:4]:
                    print('   O| ' + t.rstrip()[:120])
                if len(b['ours']) > 4:
                    print('   O| ...')
                for t in b['theirs'][:4]:
                    print('   T| ' + t.rstrip()[:120])
                if len(b['theirs']) > 4:
                    print('   T| ...')
            print(f'\n{path}: конфликтов {n}')
            continue

        if cmd not in ('ours', 'theirs'):
            raise SystemExit(__doc__)

        result = []
        for b in blocks:
            if b['kind'] == 'text':
                result.append(b['text'])
            else:
                result.extend(b['ours'] if cmd == 'ours' else b['theirs'])

        with open(path, 'w', encoding='utf-8', newline='') as f:
            f.writelines(result)
        print(f'{path}: разрешено в пользу {cmd}, осталось маркеров {count_markers(path)}')


if __name__ == '__main__':
    sys.exit(main() or 0)
