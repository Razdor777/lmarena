#!/usr/bin/env python3
"""Аудит сигнатур/оффсетов между деревьями клиента.

Читает SigManager.hpp / OffsetProvider.hpp из перечисленных деревьев и
строит таблицу: есть ли сигнатура в целевом дереве, кто её использует,
и где она встречается в исходниках.

Использование:
    python3 tools/sig_audit.py --target 26.62/src [--base src] [--out docs/]
"""
import argparse
import json
import os
import re
import sys

SIG_RE = re.compile(r'DEFINE_SIG\(\s*([A-Za-z_]\w*)')
OFF_RE = re.compile(r'DEFINE_(?:INDEX_)?FIELD\(\s*([A-Za-z_]\w*)')
# То же, но с переходом через перенос строки: в корне часть DEFINE_SIG
# записана как "DEFINE_SIG(\n  Name,", и построчный поиск её не видел.
SIG_RE_ML = re.compile(r'DEFINE_SIG\(\s*([A-Za-z_]\w*)', re.S)
OFF_RE_ML = re.compile(r'DEFINE_(?:INDEX_)?FIELD\(\s*([A-Za-z_]\w*)', re.S)
VER_RE = re.compile(r'//\s*([0-9]{1,2}\.[0-9]{1,2}\.[0-9]{1,3}[xX]?[0-9]*[^/\n]*)')

SRC_EXT = ('.cpp', '.hpp', '.h')


def read(path):
    with open(path, encoding='utf-8', errors='ignore') as fh:
        return fh.read()


def strip_comments(text):
    """Убирает //-комментарии, сохраняя переводы строк (номера строк не едут).

    Блочные /* */ не трогаем: в этих файлах их нет, а наивная реализация
    легко съедает содержимое строковых литералов.
    """
    return '\n'.join(line.split('//')[0] for line in text.split('\n'))


def collect(path, regex):
    """name -> dict(version, line). Комментарии игнорируются.

    Разбор идёт в два шага, чтобы одинаково понимать оба стиля записи:

        DEFINE_SIG(Name, "...", SigType::Sig, 0);
        DEFINE_SIG(\n   Name,
                  "...", SigType::Sig, 0);

    Шаг 1: находим все вхождения слова DEFINE_SIG (вне комментариев).
    Шаг 2: от него ищем первую '(' и первое имя внутри — но не дальше
    следующего DEFINE_SIG. Так «съесть» соседнюю запись невозможно.
    """
    out = {}
    if not os.path.exists(path):
        return out
    raw = read(path)
    clean = strip_comments(raw)
    raw_lines = raw.split('\n')
    macro = re.compile(r'\bDEFINE_(?:SIG|INDEX_FIELD|INDEX_FIELD_TYPED|FIELD)\b')

    for m in macro.finditer(clean):
        open_paren = clean.find('(', m.end())
        if open_paren == -1:
            continue
        nxt = macro.search(clean, m.end())
        stop = nxt.start() if nxt else len(clean)
        head = clean[open_paren + 1:stop]
        hit = re.search(r'[A-Za-z_][A-Za-z0-9_]*', head)
        if not hit:
            continue
        name = hit.group(0)
        line = clean.count('\n', 0, open_paren) + 1
        # Версию-пометку ищем в комментарии справа на той же строке.
        raw_line = raw_lines[line - 1] if line - 1 < len(raw_lines) else ''
        ver = VER_RE.search(raw_line.split('//', 1)[1]) if '//' in raw_line else None
        out.setdefault(name, dict(version=ver.group(1).strip() if ver else '',
                                  line=line))
    return out


def walk_sources(root):
    for base, _d, files in os.walk(root):
        if '/.git' in base:
            continue
        for f in files:
            if f.endswith(SRC_EXT):
                yield os.path.join(base, f)


def usage_index(root, names):
    """name -> [файлы], где имя встречается вне самого SigManager/OffsetProvider."""
    idx = {n: [] for n in names}
    needles = list(names)
    for path in walk_sources(root):
        if path.endswith(('SigManager.hpp', 'OffsetProvider.hpp')):
            continue
        text = read(path)
        rel = os.path.relpath(path, root)
        for n in needles:
            if n in text and re.search(r'\b' + re.escape(n) + r'\b', text):
                idx[n].append(rel)
    return idx


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--target', default='26.62/src')
    ap.add_argument('--base', default='src')
    ap.add_argument('--out', default='docs')
    args = ap.parse_args()

    t_sig = collect(os.path.join(args.target, 'SDK/SigManager.hpp'), SIG_RE)
    b_sig = collect(os.path.join(args.base, 'SDK/SigManager.hpp'), SIG_RE)
    t_off = collect(os.path.join(args.target, 'SDK/OffsetProvider.hpp'), OFF_RE)
    b_off = collect(os.path.join(args.base, 'SDK/OffsetProvider.hpp'), OFF_RE)

    t_use = usage_index(args.target, list(t_sig))
    b_use = usage_index(args.base, list(b_sig))

    rows = []
    for name in sorted(set(t_sig) | set(b_sig)):
        in_t = name in t_sig
        in_b = name in b_sig
        users = sorted(set(t_use.get(name, []) + b_use.get(name, [])))
        # кто реально пользуется (вне SDK-таблиц и вне самих себя)
        real = [u for u in users
                if not u.startswith(('SDK/Minecraft', 'Solstice', 'Dexko'))]
        rows.append(dict(name=name, in_target=in_t, in_base=in_b,
                         target_version=t_sig.get(name, {}).get('version', ''),
                         base_version=b_sig.get(name, {}).get('version', ''),
                         users=users, real_users=real))

    json.dump(rows, open(os.path.join(args.out, 'signatures.json'), 'w'),
              ensure_ascii=False, indent=1)

    def table(title, items, kind):
        lines = [f'# {title}', '',
                 '| сигнатура | в 26.62 | версия в 26.62 | у тебя (1.21.44) | '
                 'кто использует |', '|---|---|---|---|---|']
        for r in items:
            who = ', '.join(r['real_users'][:3]) or '—'
            if len(r['real_users']) > 3:
                who += f' (+{len(r["real_users"]) - 3})'
            lines.append(
                f'| `{r["name"]}` | {"да" if r["in_target"] else "**нет**"} | '
                f'{r["target_version"] or "—"} | '
                f'{"да" if r["in_base"] else "нет"} | {who} |')
        return lines

    missing = [r for r in rows if r['in_base'] and not r['in_target']]
    unused = [r for r in missing if not r['real_users']]
    used = [r for r in missing if r['real_users']]
    new = [r for r in rows if r['in_target'] and not r['in_base']]

    md = ['# Сигнатуры и оффсеты: 1.21.44 → 1.26.62', '',
          f'Источник данных: `{args.base}/SDK/*` (твой Solstice) и '
          f'`{args.target}/SDK/*` (база 26.62). Скрипт: `tools/sig_audit.py`.', '',
          f'- сигнатур в корне: **{len(b_sig)}**, в 26.62: **{len(t_sig)}**;',
          f'- оффсетов/индексов в корне: **{len(b_off)}**, в 26.62: **{len(t_off)}**;',
          f'- нет в 26.62, но есть у тебя: **{len(missing)}** '
          f'(из них используются в коде: **{len(used)}**, висят без дела: **{len(unused)}**);',
          f'- появилось нового в 26.62: **{len(new)}**;', '']
    md += ['> **Важно:** отметка «да» означает только то, что запись есть в таблице',
           '> 26.62. Подтверждена ли она именно на `1.26.62`, из кода не видно —',
           '> см. колонку «версия в 26.62» и раздел «Что требуется от тебя».', '']
    md += table('Чего нет в 26.62, но использует код (чинить в первую очередь)', used, 'sig')
    md += ['']
    md += table('Чего нет в 26.62 и никто не использует (можно удалить)', unused, 'sig')
    md += ['']
    md += table('Новое в базе 26.62', new, 'sig')

    with open(os.path.join(args.out, 'SIGNATURE_TODO_RU.md'), 'w',
              encoding='utf-8') as fh:
        fh.write('\n'.join(md) + '\n')

    print(f'сигнатур: base={len(b_sig)} target={len(t_sig)}; '
          f'нет в target={len(missing)} (используются {len(used)}, '
          f'без дела {len(unused)}); новых={len(new)}')
    print(f'оффсеты: base={len(b_off)} target={len(t_off)}')
    print(f'записано: {args.out}/SIGNATURE_TODO_RU.md, {args.out}/signatures.json')
    return 0


if __name__ == '__main__':
    sys.exit(main())
