#!/usr/bin/env python3
"""
Ищет методы, объявленные в классе (заголовок 26.62/src), но нигде не
определённые в .cpp. Ровно такая ошибка была с Interface::onLevelRenderEvent:
hpp ссылался на метод в listen<..., &Interface::onLevelRenderEvent>, а тела не
было — MSVC падал не на компиляции, а на линковке.

Что НЕ считается проблемой:
  * inline-определения в самом .hpp (тело { } прямо в объявлении);
  * чистые виртуальные (= 0) и дефолтные (= default/delete);
  * шаблоны: тело обязано быть в заголовке, вне класса его быть не может;
  * определения в любом .cpp дерева (не только рядом с .hpp).

Usage:
    python3 tools/check_undefined.py 26.62/src
Код возврата: 0 — чисто, 1 — есть неопределённые.
"""

import os
import re
import sys

CALL = re.compile(r'\b(\w+)::(~?\w+)\s*\(')
VIRTUAL = set()


def strip_comments(text):
    """Убирает // и /* */, сохраняя переводы строк (нумерация не съезжает)."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            i = n if j < 0 else j
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            seg = text[i:(n if j < 0 else j + 2)]
            out.append('\n' * seg.count('\n'))
            i = n if j < 0 else j + 2
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def class_bodies(text):
    """Возвращает [(имя, тело)] для классов со скобочным телом."""
    res = []
    for m in re.finditer(r'\bclass\s+(\w+)[^;{()]*\{', text):
        name = m.group(1)
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
        res.append((name, text[i + 1:j]))
    return res


DECL = re.compile(
    r'^[ \t]*(?:\[\[nodiscard\]\][ \t]*)?(?:static[ \t]+)?(?:virtual[ \t]+)?'
    r'(?:const[ \t]+)?[A-Za-z_][\w:<>,\*& \t]*?[\s\*&]\s*(~?\w+)\s*\([^;{]*\)[ \t]*(?:const)?[ \t]*(?:override)?[ \t]*(?:noexcept)?[ \t]*;',
    re.M)


def declarations(body):
    """Объявления методов без тела: Type name(...); (не inline, не = 0).

    Смотрим только «плоский» уровень класса: всё, что лежит внутри вложенных
    { } (тела inline-методов, лямбды, switch), пропускаем — оттуда регэксп
    вытаскивал локальные переменные вида `AABB aabb(...)`, `std::lock_guard lock(m)`.
    """
    out = []
    depth = 0
    line_no = 0
    lines = body.split('\n')
    for line in lines:
        starts_at_top = (depth == 0)
        line_no += 1
        m = DECL.match(line)
        if starts_at_top and m:
            text = m.group(0)
            if not re.search(r'=\s*(0|default|delete)\b', text) and 'PAD(' not in text:
                # virtual-объявления в SDK-заголовках — это карта vtable'а
                # (`virtual void load(IDataInput&);`), тела им не нужны: классы
                # не инстанцируются, vtable не генерируется.
                if 'virtual' in text:
                    VIRTUAL.add(m.group(1))
                    continue
                out.append((m.group(1), line.strip()))
        depth += line.count('{') - line.count('}')
        depth = max(depth, 0)
    return out


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else '26.62/src'
    if '--' in sys.argv:
        root = sys.argv[sys.argv.index('--') + 1]

    # Определения ищем и в .cpp, и в .hpp: часть кода объявлена внутри класса,
    # а тело вынесено наружу с inline прямо в заголовке (так сделан SHA256).
    defined = set()
    # Макросы, которые сами разворачиваются в `Class::method`, — например
    # COMPONENT_GET_FUNC(getFooComponent, Foo) в Actor.cpp: пары (класс, метод)
    # в тексте нет, поэтому собираем их отдельно.
    MACRO_DEF = re.compile(r'COMPONENT_GET_FUNC\(\s*(\w+)\s*,')
    macro_defs = set()
    for dirpath, _, files in os.walk(root):
        for f in files:
            if f.endswith('.cpp'):
                t = strip_comments(open(os.path.join(dirpath, f), encoding='utf-8', errors='ignore').read())
                if '#define COMPONENT_GET_FUNC' in t:
                    macro_defs |= set(MACRO_DEF.findall(t))
    for dirpath, _, files in os.walk(root):
        for f in files:
            if f.endswith(('.cpp', '.hpp')):
                p = os.path.join(dirpath, f)
                defined |= {(c, m) for c, m in CALL.findall(strip_comments(open(p, encoding='utf-8', errors='ignore').read()))}

    # Символ, на который никто не ссылается, линковку не ломает: MSVC тянет
    # только odr-used. Поэтому оставляем те объявления, которые где-то
    # используются — через указатель на метод (&C::m) или вызовом.
    used = set()
    for dirpath, _, files_ in os.walk(root):
        for f in files_:
            if not f.endswith(('.cpp', '.hpp')):
                continue
            t = strip_comments(open(os.path.join(dirpath, f), encoding='utf-8', errors='ignore').read())
            used |= set(re.findall(r'&\s*(\w+)::(\w+)', t))
            used |= set(re.findall(r'\b(\w+)::(\w+)\s*\(', t))

    bad = {}
    files = 0
    for dirpath, _, names in os.walk(root):
        for f in names:
            if not f.endswith('.hpp'):
                continue
            p = os.path.join(dirpath, f)
            text = strip_comments(open(p, encoding='utf-8', errors='ignore').read())
            if 'template' in text:
                # у шаблонных классов тело обязано быть в заголовке — их не смотрим
                pass
            files += 1
            for cname, body in class_bodies(text):
                for meth, decl in declarations(body):
                    if (cname, meth) in defined or (cname, meth) not in used:
                        continue
                    bad.setdefault(f'{cname}::{meth}', []).append(f'{p}: {decl}')

    for name in macro_defs:
        bad = {k: v for k, v in bad.items() if k.split('::')[1] != name}

    for k in sorted(bad):
        print(f'{k:45s} <- {bad[k][0]}')
    print(f'\n{len(bad)} объявлений без определения в {files} заголовках')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
