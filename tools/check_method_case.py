#!/usr/bin/env python3
"""
Ищет вызовы `Class::method(`, где в классе есть только метод с другим
регистром (`FileUtils::getsolsticeDir` против `getSolsticeDir`).

Это типичная травма переноса: имя перепечатали, MSVC выдаёт «нет такого
члена», а grep по точному имени ничего не находит. Проверка сравнивает
регистронезависимо и сообщает пару «позвали → есть».

Usage: python3 tools/check_method_case.py [26.62/src]
Код возврата: 0 — расхождений нет, 1 — есть.
"""
import os
import re
import sys

ROOT = '26.62/src'
SKIP = {'std', 'stdc', 'sig'}


def strip(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else ROOT
    members = {}          # class -> set(names)
    for dirpath, _, files in os.walk(root):
        for f in files:
            if not f.endswith('.hpp'):
                continue
            t = strip(open(os.path.join(dirpath, f), encoding='utf-8', errors='ignore').read())
            for m in re.finditer(r'\b(?:class|struct)\s+(\w+)[^;{]*\{', t):
                i = m.end() - 1
                depth, j = 0, i
                while j < len(t):
                    if t[j] == '{':
                        depth += 1
                    elif t[j] == '}':
                        depth -= 1
                        if depth == 0:
                            break
                    j += 1
                body = t[i:j]
                members.setdefault(m.group(1), set()).update(re.findall(r'\b(\w+)\s*\(', body))
                members[m.group(1)].update(re.findall(r'\b(m\w+)\b', body))

    bad = []
    for dirpath, _, files in os.walk(root):
        for f in files:
            if not f.endswith(('.cpp', '.hpp')):
                continue
            p = os.path.join(dirpath, f)
            t = strip(open(p, encoding='utf-8', errors='ignore').read())
            for cls, meth in re.findall(r'\b(\w+)::(\w+)\s*\(', t):
                if cls in SKIP or cls not in members:
                    continue
                names = members[cls]
                if meth in names:
                    continue
                lower = {n.lower() for n in names}
                if meth.lower() in lower:
                    good = sorted(n for n in names if n.lower() == meth.lower())[0]
                    bad.append(f'{p}: {cls}::{meth} → в классе {cls}::{good}')

    for b in bad:
        print(b)
    print(f'\n{len(bad)} расхождений в регистре')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
