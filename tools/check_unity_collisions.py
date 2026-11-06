#!/usr/bin/env python3
"""
Ловит конфликты имён между .cpp, которые CMake склеивает в одну unity-единицу
(в 26.62 включён UNITY_BUILD, ~32 файла на unity_N_cxx.cxx).

Два файла с `static`-переменной/функцией или одноимённым членом анонимного
namespace дают в одной единице трансляции «переопределение символа» — ошибку,
которую не видно при чтении файлов по отдельности.

Смотрим только объявления на уровне файла: `static` внутри функции — локальная
переменная, конфликта нет (такие пропускаем).

Usage: python3 tools/check_unity_collisions.py [26.62/src] [--group=32]
"""
import os
import re
import sys

# ловим только `static` в начале объявления (плюс модификаторы)
STATIC = re.compile(r'^\s*static\b')
KEYWORDS = re.compile(r'\b(if|for|while|switch|catch|return)\b')
SPECIFIERS = {'const', 'inline', 'constexpr', 'volatile', 'static', 'unsigned',
              'signed', 'struct', 'class', 'mutable', 'thread_local', 'enum'}


def name_of(header):
    """Имя объявляемого объекта: последний идентификатор слева от '=', '(' или '{'
    (`static std::string gName = ...` → gName, а не std)."""
    s = re.split(r'=', header, 1)[0]
    s = re.split(r'[({]', s, 1)[0]
    s = re.sub(r'\[[^\]]*\]', '', s)
    toks = [t for t in re.findall(r'[A-Za-z_]\w*', s) if t not in SPECIFIERS]
    return toks[-1] if toks else None


def strip_code(text):
    """Убирает комментарии и содержимое строк/символов, чтобы скобки не мешали."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            while i < n and text[i] != '\n':
                i += 1
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            i += 2
            while i + 1 < n and not (text[i] == '*' and text[i + 1] == '/'):
                i += 1
            i += 2
        elif c in '"\'':
            q = c
            out.append(' ')
            i += 1
            while i < n and text[i] != q:
                i += 2 if text[i] == '\\' else 1
            i += 1
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def items(text):
    """Разбивает текст на элементы верхнего уровня: (заголовок, тело|None)."""
    buf = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == ';':
            header = ''.join(buf).strip()
            if header:
                yield header, None
            buf = []
            i += 1
        elif c == '{':
            depth, j = 1, i + 1
            while j < n and depth:
                if text[j] == '{':
                    depth += 1
                elif text[j] == '}':
                    depth -= 1
                j += 1
            yield ''.join(buf).strip(), text[i + 1:j - 1]
            buf = []
            i = j
        else:
            buf.append(c)
            i += 1


def declarations(text, file_scope=True, anon_ns=False):
    """Имена, объявленные в этой области видимости, исключая тела функций.

    anon_ns=True — мы внутри анонимного namespace: там и функции имеют
    внутреннее связывание, поэтому они тоже считаются."""
    names = set()
    for header, body in items(text):
        h = ' '.join(header.split())
        is_static = bool(STATIC.match(h))
        if body is None:                                  # объявление с `;`
            if is_static and file_scope and not KEYWORDS.search(h):
                n = name_of(h.split('static', 1)[1])
                if n:
                    names.add(n)
            continue
        if re.search(r'\bnamespace\b', h):                # namespace/struct/enum
            if re.match(r'^(inline\s+)?namespace\s*$', h):
                names |= declarations(body, file_scope=True, anon_ns=True)
            elif is_static and file_scope:
                n = name_of(h.split('static', 1)[1])
                if n:
                    names.add(n)
            continue
        if '(' in h:                                      # определение/decl с телом
            if is_static or anon_ns:
                n = name_of(h.split('static', 1)[1] if is_static else h)
                if n:
                    names.add(n)
            continue
        if is_static and file_scope:                      # static-объект с телом {..}
            n = name_of(h.split('static', 1)[1])
            if n:
                names.add(n)
    return names


def file_scope_names(path):
    return declarations(strip_code(open(path, encoding='utf-8', errors='ignore').read()))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    root = args[0] if args else '26.62/src'
    group = 32
    for a in sys.argv[1:]:
        if a.startswith('--group='):
            group = int(a.split('=')[1])

    files = sorted(os.path.join(d, f) for d, _, fs in os.walk(root)
                   for f in fs if f.endswith('.cpp'))
    own = {p: file_scope_names(p) for p in files}

    problems = 0
    for start in range(0, len(files), group):
        chunk = files[start:start + group]
        seen = {}
        for p in chunk:
            for n in sorted(own[p]):
                if n in seen and seen[n] != p:
                    print(f'unity-группа {start // group}: `{n}` — '
                          f'{os.path.relpath(seen[n], root)} и {os.path.relpath(p, root)}')
                    problems += 1
                seen.setdefault(n, p)

    print(f'\n{problems} конфликтов имён между файлами одной unity-единицы '
          f'({len(files)} файлов, по {group})')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
