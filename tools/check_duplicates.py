#!/usr/bin/env python3
"""
Ищет повторные объявления внутри одного класса: два `struct DebugRow`, два
`debugStart()`, два `int mDebugRows`.

Так выглядит склейка двух версий файла (корневой и 26.62-ной): класс
компилируется «почти», но MSVC выдаёт C2011 (переопределение типа),
C2086 (переопределение переменной) и C2535 (функция уже объявлена).

Usage:
    python3 tools/check_duplicates.py 26.62/src
Код возврата: 0 — дублей нет, 1 — есть.
"""
import os
import re
import sys

TYPE_DECL = re.compile(r'\b(?:struct|class|enum(?:\s+class)?)\s+(\w+)\s*(?::[^{;]*)?\{?\s*$')
METHOD = re.compile(r'^\s*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+)*'
                    r'[A-Za-z_][\w:<>,\*&\s]*?[\s\*&](~?\w+)\s*\([^;{]*\)\s*(?:const)?\s*'
                    r'(?:noexcept)?\s*(?:override)?\s*(?:=\s*(?:0|default|delete))?\s*[;{]')
FIELD = re.compile(r'^\s*(?:static\s+)?(?:inline\s+)?(?:const\s+|constexpr\s+)?'
                   r'[A-Za-z_][\w:<>,\*&\s]*?[\s\*&](\w+)\s*(?:\[[^\]]*\])?\s*(?:=[^;]*|\([^;]*\)|\{[^;]*\})?\s*;')


DEAD_IF = re.compile(r'#\s*if(def)?\s+(__PRIVATE_BUILD__|__DEBUG__|0)\b')
NOT_KEYWORD = {'return', 'if', 'while', 'for', 'switch', 'sizeof', 'delete', 'new'}


def drop_macros(text):
    """Вырезает #define-тела: у макросов аргумент name — это параметр, а не член
    класса (DEFINE_PATCH_FUNC(name, ...) встречается дважды и это нормально)."""
    out, i = [], 0
    for line in text.split('\n'):
        if re.match(r'\s*#\s*define\b', line):
            out.append('')
            i = 1
            continue
        if i and line.rstrip().endswith('\\'):
            out.append('')
            continue
        i = 0
        out.append(line)
    return '\n'.join(out)


def drop_private(text):
    """Оставляет только «не-приватную» ветку #ifdef __PRIVATE_BUILD__ / #if 0.

    Иначе одно и то же поле, объявленное в #ifdef и в #else, выглядит как дубль,
    хотя компилируется ровно одно из них.
    """
    out, stack = [], []          # stack: True = сейчас пишем, False = внутри мёртвой ветки
    for line in text.split('\n'):
        head = line.strip()
        if DEAD_IF.match(head):
            stack.append([False, False])   # [пишем_сейчас, видели_else]
            out.append('')
            continue
        if stack and re.match(r'#\s*if(def|ndef)?\b', head):
            stack.append([stack[-1][0], False])
            out.append('')
            continue
        if stack and re.match(r'#\s*(else|elif)\b', head):
            stack[-1] = [not stack[-1][0], True]
            out.append('')
            continue
        if stack and re.match(r'#\s*endif\b', head):
            stack.pop()
            out.append('')
            continue
        out.append(line if (not stack or stack[-1][0]) else '')
    return '\n'.join(out)


def strip(text):
    text = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def classes(text):
    """(имя, смещение_тела, тело) для всех классов с фигурными скобками."""
    # `class Foo` в списке параметров (`f(class Foo& x)`) — это предобъявление
    # или уже известный тип, а не определение: за ним нет `{`. Требуем, чтобы
    # между именем и `{` не было скобок.
    for m in re.finditer(r'\b(?:class|struct)\s+(\w+)(?:\s*:\s*[^{}();]+)?\s*\{', text):
        i = m.end() - 1
        depth, j = 0, i
        while j < len(text):
            if text[j] == '{':
                depth += 1
            elif text[j] == '}':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        yield m.group(1), i, text[i + 1:j]


def scan(path):
    raw = open(path, encoding='utf-8', errors='ignore').read()
    text = strip(drop_private(drop_macros(raw)))
    problems = []
    for cname, offset, body in classes(text):
        # класс, объявленный внутри функции/макроса (struct _SampleLol в .cpp,
        # class в #define), к склейке версий отношения не имеет — и «повтор»
        # там обычно один и тот же локальный тип в двух местах.
        before = text[:offset]
        if before.count('{') > before.count('}'):
            continue
        seen = {}
        depth = 0
        for line in body.split('\n'):
            top = depth == 0
            depth += line.count('{') - line.count('}')
            if depth < 0:
                depth = 0
            if not top:
                continue
            name = None
            m = TYPE_DECL.match(line)
            if m:
                name = ('type', m.group(1), '')
            else:
                m = METHOD.match(line)
                if m:
                    # перегрузки — это НЕ дубли: ключ включает параметры
                    params = re.search(r'\(([^;{]*)\)', line)
                    sig = re.sub(r'\s+', ' ', params.group(1) if params else '')
                    name = ('func', m.group(1), sig)
                else:
                    m = FIELD.match(line)
                    if m:
                        name = ('field', m.group(1), '')
            if not name:
                continue
            kind, nm = name[0], name[1]
            # «именем поля» не может быть число или голый тип (битовые поля
            # `Material : 1`, объявления вида `uint8_t x;` ловятся криво)
            if nm in NOT_KEYWORD or nm.isdigit() or re.fullmatch(r'\w+_t', nm):
                continue
            if name in seen:
                line_no = raw[:offset].count('\n') + 1 + body[:body.index(line)].count('\n')
                problems.append(f'{path}: {cname}::{nm} ({kind}) — повтор, первое объявление выше'
                                + (f' [{name[2]}]' if name[2] else ''))
            else:
                seen[name] = True
    return problems


def main():
    root = sys.argv[-1] if len(sys.argv) > 1 else '26.62/src'
    problems = []
    for dirpath, _, files in os.walk(root):
        for f in files:
            if f.endswith(('.hpp', '.cpp')):
                problems.extend(scan(os.path.join(dirpath, f)))

    for p in problems:
        print(p)
    print(f'\n{len(problems)} повторных объявлений в классах')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
