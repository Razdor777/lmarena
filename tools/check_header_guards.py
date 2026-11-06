#!/usr/bin/env python3
"""
Заголовки без защиты от повторного включения.

При UNITY_BUILD десятки .cpp попадают в одну единицу трансляции, и заголовок
без `#pragma once` / стража включается дважды → «переопределение класса»
(C2011), за которым следуют невнятные C2027/C2079/C2665 в чужих файлах.

Проверяются свои заголовки в 26.62/src (третья сторона — stb_*, Luau, entt,
minhook, ImGui — пропускается: там свои правила).

Usage: python3 tools/check_header_guards.py [26.62/src]
"""
import os
import re
import sys

SKIP_DIRS = {'include'}          # третья сторона лежит отдельно
SKIP_PREFIX = ('stb_',)          # stb сознательно без стража


def has_guard(text):
    if re.search(r'^[ \t]*#[ \t]*pragma[ \t]+once', text, re.M):
        return True
    m = re.search(r'^[ \t]*#[ \t]*ifndef[ \t]+(\w+)', text, re.M)
    if not m:
        return False
    return re.search(r'^[ \t]*#[ \t]*define[ \t]+' + re.escape(m.group(1)), text, re.M) is not None


def main():
    root = next((a for a in sys.argv[1:] if not a.startswith('-')), '26.62/src')
    bad = []
    total = 0
    for d, dirs, files in os.walk(root):
        dirs[:] = [x for x in dirs if x not in SKIP_DIRS]
        for f in sorted(files):
            if not f.endswith(('.hpp', '.h')) or f.startswith(SKIP_PREFIX):
                continue
            total += 1
            path = os.path.join(d, f)
            # utf-8-sig снимает BOM, иначе `#pragma` не в начале строки
            text = open(path, encoding='utf-8-sig', errors='ignore').read()
            if not has_guard(text):
                bad.append(path)

    for p in bad:
        print(f'без #pragma once: {p}')
    print(f'{len(bad)} заголовков без защиты от повторного включения (из {total})')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
