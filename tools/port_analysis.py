#!/usr/bin/env python3
"""Анализ переносимости модулей из базы 1.21.44 (src/) в базу 26.62 (26.62/src/).

Для каждого модуля смотрит:
  - что он подключает и существует ли это в 26.62;
  - какие поля OffsetProvider / сигнатуры SigManager / события он использует
    и есть ли они в 26.62;
  - к каким типам SDK он обращается.

Вывод: список модулей, отсортированный по «дешевизне» переноса.
"""
import re
import sys
import pathlib

OLD = pathlib.Path('src')
NEW = pathlib.Path('26.62/src')

INC = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
OP = re.compile(r'OffsetProvider::(\w+)')
SIG = re.compile(r'SigManager::(\w+)')
EV = re.compile(r'\b(?:listen|deafen|trigger)<\s*(\w+)')

SYSTEM = re.compile(r'^(?:[a-z_]+\.h|windows|winrt/|d3d|dxgi|dwrite|d2d|winsock|ws2|mmsystem|'
                    r'imgui|spdlog|fmt/|nlohmann/|glm/|magic_enum|libhat|kiero|minhook|nes/|'
                    r'cryptopp/|omp\.h|build_info\.h)')


def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    return text


def new_symbols(name):
    """Собирает имена сигнатур и полей, объявленных в новой базе."""
    sig = set()
    for f in (NEW / 'SDK').glob('*.hpp'):
        t = f.read_text(encoding='utf-8', errors='replace')
        sig |= set(re.findall(r'DEFINE_SIG\(\s*(\w+)', t))
        sig |= set(re.findall(r'DEFINE_INDEX_FIELD\w*\(\s*[^,]+,\s*(\w+)', t))
        sig |= set(re.findall(r'DEFINE_FIELD\(\s*(\w+)', t))
    return sig


def analyze(mod, new_syms):
    cpp, hpp = OLD / f'{mod}.cpp', OLD / f'{mod}.hpp'
    found = [p for p in (cpp, hpp) if p.exists()]
    if not found:
        # maybe it is a .hpp-only module
        found = list(OLD.rglob(f'{mod}.hpp'))
    text = ''.join(strip_comments(p.read_text(encoding='utf-8', errors='replace')) for p in found)

    missing_hdr, missing_sym, missing_ev = set(), set(), set()
    for inc in INC.findall(text):
        if SYSTEM.match(inc):
            continue
        cand = [NEW / inc, NEW / 'SDK' / inc]
        if not any(c.exists() for c in cand) and not (NEW / f'{mod}.hpp').parent.joinpath(inc).exists():
            missing_hdr.add(inc)

    for s in OP.findall(text) + SIG.findall(text):
        if s not in new_syms:
            missing_sym.add(s)

    evdir = NEW / 'Features/Events'
    for e in EV.findall(text):
        if not (evdir / f'{e}.hpp').exists():
            missing_ev.add(e)

    return {
        'mod': mod,
        'files': [str(p) for p in found],
        'hdr': sorted(missing_hdr),
        'sym': sorted(missing_sym),
        'ev': sorted(missing_ev),
    }


def main():
    mods = pathlib.Path('/tmp/migrate_mods.txt')
    names = [m.strip() for m in mods.read_text().split() if m.strip()] if mods.exists() else sys.argv[1:]
    new_syms = new_symbols(None)
    rows = [analyze(m, new_syms) for m in names]
    rows.sort(key=lambda r: (len(r['hdr']) + len(r['sym']) + len(r['ev'])))

    print(f"{'MODULE':22} {'HDR':>4} {'SYM':>4} {'EVT':>4}  ДЕТАЛИ")
    print('-' * 110)
    for r in rows:
        score = len(r['hdr']) + len(r['sym']) + len(r['ev'])
        detail = ', '.join(r['hdr'] + r['sym'] + r['ev'])[:70]
        print(f"{r['mod']:22} {len(r['hdr']):4} {len(r['sym']):4} {len(r['ev']):4}  {detail}")
        if score == 0:
            pass
    print()
    clean = [r['mod'] for r in rows if not (r['hdr'] or r['sym'] or r['ev'])]
    print(f"переносятся без правок ({len(clean)}): {' '.join(clean)}")


if __name__ == '__main__':
    main()
