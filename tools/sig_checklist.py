#!/usr/bin/env python3
"""
Чек-лист сигнатур для 26.62: что уже есть, что ждёт скана, что заведомо мертво.

Бинарника 1.26.5x под рукой нет, поэтому скрипт ничего не сканирует — он
собирает список и печатает готовый markdown, чтобы после получения exe осталось
только прогнать паттерны (IDAPython/Ghidra) и вписать адреса.

Как читается SigManager.hpp:
  * DEFINE_SIG(...)            — живая сигнатура (сканер попробует её найти);
  * // DEFINE_SIG(...)         — отключена (мертва или ждёт новой);
  * static inline uintptr_t X = 0;  — объявлена, но не определяется: детур,
    который её использует, просто не встанет (в логе будет «signature not
    resolved»), игра при этом не падает.

Usage:
    python3 tools/sig_checklist.py                    # печать отчёта
    python3 tools/sig_checklist.py --out docs/SIGS_26.62_RU.md
"""

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SIG = os.path.join(ROOT, '26.62', 'src', 'SDK', 'SigManager.hpp')
SRC = os.path.join(ROOT, '26.62', 'src')

# Сигнатуры, которые используются хуками/кодом (по имени в SigManager::).
USE = re.compile(r'SigManager::(\w+)')

# Паттерны, которых в базе 1.21.44 не было: они написаны «по форме пролога», а не
# сняты с 1.26.5x. Сканер их найдёт только если пролог совпал, поэтому в отчёте
# они помечены отдельно — при первой же возможности проверить на бинарнике.
GUESSED = {
    'Actor_hurt', 'Projectile_onHit', 'HurtColor',
    'ContainerScreenController_onContainerSlotHovered',
    'ItemStack_fromDescriptor', 'ContainerScreenController_tryBeginTransition',
}

# Группы — чтобы в отчёте было видно, что именно ломается без сигнатуры.
GROUPS = [
    ('Урон и снаряды', ['Actor_hurt', 'Projectile_onHit', 'HurtColor']),
    ('Ambience (небо/туман/облака)', ['Ambience_SkyRender', 'Ambience_EndSkyRender',
                                      'Ambience_ChunkRender', 'Ambience_SunMoonRender',
                                      'Ambience_CloudRender']),
    ('Инвентарь', ['ContainerScreenController_tick',
                   'ContainerScreenController_onContainerSlotHovered',
                   'ContainerScreenController_handleAutoPlace',
                   'ContainerScreenController_tryBeginTransition']),
    ('Предметы', ['ItemStack_fromDescriptor', 'EnchantUtils_getEnchantLevel',
                  'GameMode_getDestroyRate', 'GameMode_baseUseItem']),
    ('Сеть', ['ComplexInventoryTransaction_vtable', 'ConnectionRequest_create_DefaultInputMode']),
    ('Анимации/рендер', ['BobHurt', 'TapSwingAnim', 'FluxSwing', 'LevelRenderer_renderLevel']),
]


def parse():
    live, off, stubs = {}, {}, {}
    text = open(SIG, encoding='utf-8', errors='ignore').read()
    text_lines = text.split('\n')
    for i, line in enumerate(text_lines):
        code = line.split('//')[0]
        # многострочный DEFINE_SIG: имя на одной строке, паттерн на следующей
        flat = ' '.join(l.strip() for l in text_lines[i:i + 4])
        m = re.search(r'DEFINE_SIG\((\w+)\s*,\s*"([^"]*)"', code)
        if not m:
            m = re.search(r'DEFINE_SIG\((\w+)\s*,\s*"([^"]*)"', flat)
        if m:
            live[m.group(1)] = m.group(2)
            continue
        m = re.search(r'//\s*DEFINE_SIG\((\w+)\s*,\s*"([^"]*)"', line)
        if m:
            off[m.group(1)] = m.group(2)
            continue
        m = re.search(r'static\s+inline\s+uintptr_t\s+(\w+)\s*=\s*0\s*;', code)
        if m:
            stubs[m.group(1)] = True
    return live, off, stubs


def users():
    used = {}
    for dirpath, _, files in os.walk(SRC):
        for f in files:
            if not f.endswith(('.cpp', '.hpp')):
                continue
            p = os.path.join(dirpath, f)
            try:
                text = open(p, encoding='utf-8', errors='ignore').read()
            except OSError:
                continue
            for name in USE.findall(text):
                used.setdefault(name, set()).add(os.path.relpath(p, SRC))
    return used


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default=None)
    a = ap.parse_args()

    live, off, stubs = parse()
    used = users()

    out = []
    w = out.append
    w('# Сигнатуры 26.62: состояние и чек-лист\n')
    w('Сгенерировано `tools/sig_checklist.py`. Бинарника 1.26.5x нет, поэтому')
    w('числа ниже — это не «найдено/не найдено», а «что ждёт скана».\n')
    w(f'- живых `DEFINE_SIG`: **{len(live)}**')
    w(f'- закомментированных: **{len(off)}**')
    w(f'- объявленных заглушек (`= 0`, детур не встанет): **{len(stubs)}**')
    w(f'- имён, на которые реально ссылается код: **{len(used)}**\n')

    w('## Что сломается без сигнатуры\n')
    w('| Группа | Имя | Состояние | Где используется |')
    w('|---|---|---|---|')
    for title, names in GROUPS:
        for n in names:
            if n in live and n in GUESSED:
                state = '🟡 паттерн-догадка (проверить)'
            elif n in live:
                state = '✅ есть паттерн'
            elif n in stubs:
                state = '⛔ заглушка `= 0`'
            elif n in off:
                state = '🟡 отключена (закомментирована)'
            else:
                state = '❔ не объявлена'
            where = ', '.join(sorted(used.get(n, []))) or '—'
            w(f'| {title} | `{n}` | {state} | {where} |')
    w('')

    # Только настоящие сигнатуры: SigManager::initialize/scanSig/mSigs — это
    # методы самого менеджера, а не паттерны.
    known = set(live) | set(off) | set(stubs)
    # Заглушки Ambience (пять `= 0`) специально не считаем «нет паттерна»:
    # это осознанное решение (см. комментарий в SigManager), они в отдельной
    # таблице выше. Остальное — реальные дыры.
    missing_used = sorted(n for n in used
                          if n in known and n not in live and n not in stubs)
    w('## Код зовёт, а живого паттерна нет\n')
    if missing_used:
        w('| Имя | Почему нет | Где |')
        w('|---|---|---|')
        for n in missing_used:
            why = 'заглушка `= 0`' if n in stubs else 'паттерн закомментирован'
            w(f'| `{n}` | {why} | {", ".join(sorted(used[n]))} |')
    else:
        w('нет — всё, что зовёт код, имеет живой паттерн')
    w('')
    w('## Порядок скана, когда появится exe\n')
    w('0. Сначала перепроверить шесть «догадок» — они уже вписаны, но не сняты:')
    w('   `Actor_hurt`, `Projectile_onHit`, `HurtColor`,')
    w('   `ContainerScreenController_onContainerSlotHovered`,')
    w('   `ItemStack_fromDescriptor`, `ContainerScreenController_tryBeginTransition`.')
    w('   Если пролог не совпал — сканер вернёт 0, и детур не встанет (падения нет).')
    w('')
    w('1. Снять `Actor_hurt` — без него молчат `DamageLog` и `LastTP`.')
    w('2. Снять `Projectile_onHit` — без него молчат `BowHitMarker` и `ArrowRecovery`.')
    w('3. Снять `HurtColor` — без него `HitColor` не перекрашивает вспышку урона.')
    w('4. Снять `ContainerScreenController_onContainerSlotHovered` — без него')
    w('   `JavaInventory`/`JavaInventoryHotkeys` не знают слот под курсором.')
    w('5. Снять пять `Ambience_*` — без них `Ambience` молчит (небо/туман/облака).')
    w('6. Проверить «унаследованные» `ItemStack_fromDescriptor` и')
    w('   `ContainerScreenController_tryBeginTransition` — они восстановлены по')
    w('   догадке, а не сняты с бинарника.\n')

    text = '\n'.join(out) + '\n'
    if a.out:
        open(a.out, 'w', encoding='utf-8', newline='\n').write(text)
        print(f'written: {a.out}', file=sys.stderr)
    else:
        print(text)
    return 0


if __name__ == '__main__':
    sys.exit(main())
