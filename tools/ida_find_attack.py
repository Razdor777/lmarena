# -*- coding: utf-8 -*-
"""
ida_find_attack.py — найти функцию удара (и проверки критов) в любом билде 1.26.x.

Отвечает на вопрос «тот это код или нет» двумя независимыми способами и
сверяет результаты:

  A. По структуре. Те же условия, что и в самом клиенте (CritPatchUtils):
     ищется цепочка условных переходов, у которой цель «плохих» веток — ровно
     байт за последним переходом, а сам последний уходит далеко вперёд.
     Если тут что-то нашлось — клиент найдёт это же и патч применится.

  B. По строкам. Функция удара почти всегда держит в себе имена звуков удара
     и/или партиклов крита. Печатаются все функции, которые ссылаются на них.

  C. Вердикт: если A и B указывают на одну и ту же функцию — это она.

Запуск: File -> Script file... (Alt+F7). Ничего не патчит.

Строки-ориентиры (ищутся подстрокой, регистр не важен):
  game.player.attack.strong / .weak / .nodamage
  critical_hit_emitter, critical_hit, minecraft:critical
"""

import re

try:
    import idaapi
    import ida_bytes
    import ida_funcs
    import ida_segment
    import idautils
    import idc
    HAVE_IDA = True
except ImportError:
    HAVE_IDA = False

ANCHOR_STRINGS = [
    # звуки удара — есть не во всех билдах
    "game.player.attack.strong",
    "game.player.attack.weak",
    "game.player.attack.nodamage",
    "critical_hit_emitter",
    "critical_hit",
    "critical",
    "entity_attack",
    # самый живучий якорь: путь исходника в assert'ах Player::*
    "player\\player.cpp",
    "player.cpp",
]

MAX_BACK = 0x400        # насколько вверх смотрим соседей (в 1.26.51 хватало 0x169)
MIN_SITES = 5           # меньше — это не наша цепочка
MIN_CRIT_GAP = 0x80     # цель последнего перехода должна быть не ближе
MAX_CRIT_GAP = 0x2000   # ... и не дальше


def _addr(base, off):
    return base + off


# ─────────────────────────────────────────────────────────────────────────────
# A. Структурный поиск
# ─────────────────────────────────────────────────────────────────────────────

def find_structural(data, base):
    near_re = re.compile(rb"\x0f[\x80-\x8f]")
    short_re = re.compile(rb"[\x70-\x7f]")

    found = []

    for m in near_re.finditer(data):
        i = m.start()
        if i + 6 > len(data):
            continue

        rel = int.from_bytes(data[i + 2:i + 6], "little", signed=True)
        at = base + i
        fall = at + 6
        target = fall + rel

        if target <= fall + MIN_CRIT_GAP or target > fall + MAX_CRIT_GAP:
            continue

        lo = i - MAX_BACK if i > MAX_BACK else 0
        window = data[lo:i]
        sites = []

        for sm in short_re.finditer(window):
            j = lo + sm.start()
            if j + 2 > len(window):
                continue
            disp = window[sm.start() + 1]
            if disp >= 0x80:
                disp -= 0x100
            if base + j + 2 + disp == fall:
                sites.append(j)

        for nm in near_re.finditer(window):
            j = lo + nm.start()
            if j + 6 > len(window):
                continue
            d = int.from_bytes(window[nm.start() + 2:nm.start() + 6], "little", signed=True)
            if base + j + 6 + d == fall:
                sites.append(j)

        if len(sites) < MIN_SITES:
            continue

        sites.sort()
        overlap = any(sites[k] < sites[k - 1] + 2 for k in range(1, len(sites)))
        if overlap:
            continue

        found.append({
            "anchor": i,
            "sites": sites,
            "abort": fall,
            "crit": target,
        })

    found.sort(key=lambda f: (-len(f["sites"]), f["anchor"]))
    return found


# ─────────────────────────────────────────────────────────────────────────────
# B. Поиск по строкам
# ─────────────────────────────────────────────────────────────────────────────

def find_by_strings(needles):
    by_string = {}
    funcs = {}

    try:
        strings = list(idautils.Strings())
    except Exception as exc:      # noqa: BLE001 — строк много, бывает всякое
        print("[!] idautils.Strings() упал: %s" % exc)
        strings = []

    for item in strings:
        try:
            text = str(item)
        except Exception:
            continue
        low = text.lower()

        for needle in needles:
            if needle not in low:
                continue

            for xref in idautils.XrefsTo(item.ea, 0):
                func = ida_funcs.get_func(xref.frm)
                if func is None:
                    continue
                by_string.setdefault(needle, set()).add(func.start_ea)
                funcs.setdefault(func.start_ea, set()).add(needle)
            break

    return by_string, funcs


# ─────────────────────────────────────────────────────────────────────────────

def main():
    seg = ida_segment.get_segm_by_name(".text") or ida_segment.get_first_seg()
    if seg is None:
        print("[!] нет сегмента кода")
        return

    start, end = seg.start_ea, seg.end_ea
    data = ida_bytes.get_bytes(start, end - start)
    if not data:
        print("[!] не удалось прочитать .text")
        return

    imagebase = idaapi.get_imagebase()
    print("[*] .text: %X..%X (%d МБ), imagebase %X"
          % (start, end, len(data) // (1024 * 1024), imagebase))

    # ── B сначала: он даёт маленький список кандидатов, и структуру ищем уже в них
    print("\n=== B. ПО СТРОКАМ ===")
    by_string, funcs = find_by_strings(ANCHOR_STRINGS)
    if not by_string:
        print("  ни одна из строк-ориентиров не найдена "
              "(в этом билде могут быть другие имена — скажи, поищем по твоим)")
    for needle, addrs in sorted(by_string.items()):
        for ea in sorted(addrs):
            func = ida_funcs.get_func(ea)
            size = (func.end_ea - func.start_ea) if func else 0
            print("  %-28s -> rva=%X size=0x%X %s"
                  % (needle, ea - imagebase, size, idc.get_func_name(ea)))

    # ── A
    print("\n=== A. ПО СТРУКТУРЕ ===")
    hits = []
    if funcs:
        print("  ищем только внутри функций-кандидатов (%d шт.)" % len(funcs))
        for fstart in sorted(funcs):
            func = ida_funcs.get_func(fstart)
            if func is None:
                continue
            body = ida_bytes.get_bytes(func.start_ea, func.end_ea - func.start_ea) or b""
            if not body:
                continue
            for hit in find_structural(body, func.start_ea)[:2]:
                hits.append(hit)
        hits.sort(key=lambda h: (-len(h["sites"]), h["anchor"]))
    else:
        print("  кандидатов по строкам нет — сканируем весь .text (это дольше)")
        hits = find_structural(data, start)

    if not hits:
        print("  цепочек не найдено")
    for idx, hit in enumerate(hits[:6], 1):
        ea = start + hit["anchor"]
        func = ida_funcs.get_func(ea)
        f_rva = (func.start_ea - imagebase) if func else -1
        f_size = (func.end_ea - func.start_ea) if func else 0
        base_ea = start + hit["anchor"]
        rel = [site_ea - base_ea for site_ea in hit["sites"]]
        print("  #%d  anchor_rva=%X  переходов в «не крит»: %d  цель крита: +0x%X"
              % (idx, ea - imagebase, len(hit["sites"]), hit["crit"] - (start + hit["anchor"])))
        print("      отпечаток (смещения сайтов от anchor): %s"
              % " ".join("-0x%X" % (-r) if r < 0 else "+0x%X" % r for r in rel))
        print("      функция: rva=%X size=0x%X name=%s"
              % (f_rva, f_size, idc.get_func_name(func.start_ea) if func else "?"))

    # ── C
    print("\n=== C. ВЕРДИКТ ===")
    best = None
    for fstart, needles in funcs.items():
        if best is None or len(needles) > len(funcs[best]):
            best = fstart

    if best is not None:
        print("  больше всего ориентиров в функции rva=%X (%d из %d): %s"
              % (best - imagebase, len(funcs[best]), len(ANCHOR_STRINGS),
                 idc.get_func_name(best)))
        print("  строки: %s" % ", ".join(sorted(funcs[best])))

    match = None
    for hit in hits:
        ea = start + hit["anchor"]
        func = ida_funcs.get_func(ea)
        if func and func.start_ea == best:
            match = hit
            break

    if match is not None:
        print("\n  ИТОГ: структура и строки указывают на одну функцию (rva=%X)." % (best - imagebase))
        print("        Это она. Клиент найдёт её сам: Criticals -> Mode = Native.")
    elif hits:
        print("\n  ИТОГ: структурное совпадение есть (rva=%X), но строки указывают в другое место."
              % ((ida_funcs.get_func(start + hits[0]["anchor"]).start_ea - imagebase)
                 if ida_funcs.get_func(start + hits[0]["anchor"]) else -1))
        print("        Проверь глазами: рядом с найденным anchor должны быть вызовы урона/партиклов.")
    elif best is not None:
        print("\n  ИТОГ: структуры нет, но кандидат по строкам — rva=%X." % (best - imagebase))
        print("        Пришли его листинг: разберу, как проверять крит в этом билде.")
    else:
        print("\n  ИТОГ: ни структуры, ни строк. Пришли листинг Player::attack "
              "(найди через vtable актора или по строкам урона).")


if __name__ == "__main__":
    if HAVE_IDA:
        main()
    else:
        print("Запускать внутри IDA: File -> Script file...")
