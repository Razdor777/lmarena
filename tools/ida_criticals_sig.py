# -*- coding: utf-8 -*-
"""
ida_criticals_sig.py — поиск функции «критов» в IDA и генерация сигнатур.

Зачем: рабочий модуль Criticals (совместимый с игрой 1.26.51) правит 8 мест в
одной большой функции обработки удара по фиксированным RVA. RVA у каждой
сборки свои, поэтому для 1.26.10 их нужно найти заново и превратить в
сигнатуры, которые клиент умеет искать сам (libhat-формат, как в
src/SDK/SigManager.hpp).

Функция узнаётся по хвосту — три коротких условных перехода с маленьким шагом
и сразу за ними дальний jcc:

    +0x34D   72 2D              jb   short +2D
    +0x35F   75 1B              jne  short +1B
    +0x36C   75 0E              jne  short +0E
    +0x376   0F 85 96 03 00 00  jne  near  +396

Выше по функции, в пределах ~0x300 байт, должны быть ещё четыре дальних jcc
(в 1.26.51 это +0x1E4, +0x1F8, +0x29D, +0x2AD).

Запуск в IDA: File -> Script file... (Alt+F7), выбрать этот файл.
Ничего не патчит, только печатает в Output window.

Вывод:
  0. вердикт: совпадает ли структура с эталоном (если да — патч в клиенте найдёт
     её сам, без сигнатур: Criticals -> Mode = Native);
  1. кандидаты (RVA функции, размер, шаги кластера);
  2. libhat-сигнатуру начала функции — уникальную в пределах модуля;
  3. готовую строку DEFINE_SIG(...) для SigManager.hpp;
  4. таблицу всех условных переходов внутри функции с их целями
     (по ней видно, какие ветки ведут в «не крит», а какая — в «крит»);
  5. сравнение со смещениями эталонной сборки 1.26.51.
"""

import re
from bisect import bisect_right

try:
    import idaapi
    import ida_bytes
    import ida_funcs
    import ida_segment
    import ida_ua
    import idc
    HAVE_IDA = True
except ImportError:
    HAVE_IDA = False


# ─────────────────────────────────────────────────────────────────────────────
# Эталон 1.26.51 (0x2FB8DE0). Нужен только для сверки, в патч не идёт.
# ─────────────────────────────────────────────────────────────────────────────

REF_FUNC_RVA = 0x2FB8DE0
REF_FUNC_SIZE = 0xEEE

# (смещение от начала функции, оригинальные байты)
REF_SITES = [
    (0x1E4, bytes.fromhex("0F 85 92 01 00 00")),
    (0x1F8, bytes.fromhex("0F 86 7E 01 00 00")),
    (0x29D, bytes.fromhex("0F 82 D9 00 00 00")),
    (0x2AD, bytes.fromhex("0F 85 C9 00 00 00")),
    (0x34D, bytes.fromhex("72 2D")),
    (0x35F, bytes.fromhex("75 1B")),
    (0x36C, bytes.fromhex("75 0E")),
    (0x376, bytes.fromhex("0F 85 96 03 00 00")),
]


# ─────────────────────────────────────────────────────────────────────────────
# Работа с байтами — без зависимостей от IDA (можно тестировать отдельно).
# ─────────────────────────────────────────────────────────────────────────────

# Любой короткий условный переход: 70..7F.
_SHORT_JCC_RE = re.compile(rb"[\x70-\x7f]")
# Любой дальний условный переход: 0F 80..8F.
_NEAR_JCC_RE = re.compile(rb"\x0f[\x80-\x8f]")


def find_clusters(data, gap_max=0x30):
    """
    Ищет тройки коротких jcc в пределах gap_max друг от друга, за которыми
    сразу идёт дальний jcc. Возвращает [(i, j-i, k-j, q-k), ...] — смещения
    относительно первого перехода кластера.
    """
    short = [m.start() for m in _SHORT_JCC_RE.finditer(data)]
    near = [m.start() for m in _NEAR_JCC_RE.finditer(data)]

    out = []
    for idx, i in enumerate(short):
        end1 = bisect_right(short, i + gap_max)
        for a2 in range(idx + 1, end1):
            j = short[a2]
            end2 = bisect_right(short, j + gap_max)
            for a3 in range(a2 + 1, end2):
                k = short[a3]
                first_near = bisect_right(near, k)
                if first_near >= len(near) or near[first_near] - k > gap_max:
                    continue
                out.append((i, j - i, k - j, near[first_near] - k))
    return out


def count_matches(pattern, data):
    """
    Сколько раз pattern (список int | None) встречается в data.
    None возвращается, если в шаблоне меньше трёх подряд известных байт —
    такой шаблон бесполезно считать «уникальным».
    """
    n = len(pattern)

    best_len, best_off = 0, 0
    run_len, run_off = 0, 0
    for idx, value in enumerate(pattern):
        if value is None:
            run_len = 0
            continue
        if run_len == 0:
            run_off = idx
        run_len += 1
        if run_len > best_len:
            best_len, best_off = run_len, run_off

    if best_len < 3:
        return None

    anchor = bytes(pattern[best_off:best_off + best_len])
    count = 0
    pos = 0
    while True:
        hit = data.find(anchor, pos)
        if hit < 0:
            break
        base = hit - best_off
        if 0 <= base and base + n <= len(data):
            ok = True
            for t in range(n):
                value = pattern[t]
                if value is not None and data[base + t] != value:
                    ok = False
                    break
            if ok:
                count += 1
        pos = hit + 1
    return count


def to_libhat(pattern):
    return " ".join("?" if v is None else "%02X" % v for v in pattern)


def wrap_cpp(text, width=88, indent="             "):
    parts = text.split(" ")
    lines, line = [], ""
    for part in parts:
        candidate = part if not line else line + " " + part
        if len(candidate) > width and line:
            lines.append(line)
            line = part
        else:
            line = candidate
    if line:
        lines.append(line)
    return "\n".join(indent + '"' + l + ' "' for l in lines[:-1]) + "\n" + indent + '"' + lines[-1] + '"'


# ─────────────────────────────────────────────────────────────────────────────
# Ниже — только IDA.
# ─────────────────────────────────────────────────────────────────────────────

def _decode(ea):
    insn = ida_ua.insn_t()
    if ida_ua.decode_insn(insn, ea) <= 0:
        return None
    return insn


def _insn_pattern(ea):
    """Байты одной инструкции с None на относительных/абсолютных операндах."""
    insn = _decode(ea)
    if insn is None:
        return None, 0
    raw = ida_bytes.get_bytes(ea, insn.size)
    if raw is None or len(raw) != insn.size:
        return None, 0

    # Операнд у x86 всегда в конце инструкции, поэтому «до конца» — безопасно.
    wild = set()
    for op in insn.ops:
        if op.type in (ida_ua.o_near, ida_ua.o_far, ida_ua.o_mem, ida_ua.o_imm):
            wild.update(range(op.offb, insn.size))

    return [None if k in wild else raw[k] for k in range(insn.size)], insn.size


def build_pattern(ea_from, ea_to):
    pattern = []
    ea = ea_from
    while ea < ea_to:
        chunk, size = _insn_pattern(ea)
        if chunk is None:
            return None
        pattern.extend(chunk)
        ea += size
    return pattern


def unique_signature(ea_start, data, limit=0xC0):
    """
    Наращивает сигнатуру от ea_start по инструкциям, пока она не станет
    единственной в модуле. Возвращает (pattern, длина_в_байтах) или None.
    """
    pattern = []
    ea = ea_start
    while len(pattern) < limit:
        chunk, size = _insn_pattern(ea)
        if chunk is None:
            return None
        pattern.extend(chunk)
        ea += size

        if len(pattern) < 12:
            continue

        count = count_matches(pattern, data)
        if count == 1:
            return pattern, len(pattern)
        if count is None:
            continue
    return None


def jcc_report(func_start, func_end):
    """Все условные переходы функции: смещение, байты, цель."""
    rows = []
    ea = func_start
    while ea < func_end:
        insn = _decode(ea)
        if insn is None:
            break
        raw = ida_bytes.get_bytes(ea, insn.size) or b""

        if raw[:1] and 0x70 <= raw[0] <= 0x7F and len(raw) >= 2:
            disp = raw[1] - 0x100 if raw[1] >= 0x80 else raw[1]
            rows.append((ea - func_start, ea, raw, ea + 2 + disp))
        elif len(raw) >= 6 and raw[0] == 0x0F and 0x80 <= raw[1] <= 0x8F:
            disp = int.from_bytes(raw[2:6], "little", signed=True)
            rows.append((ea - func_start, ea, raw, ea + 6 + disp))

        ea += insn.size
    return rows


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
    print("[*] .text: %X..%X, imagebase %X" % (start, end, imagebase))

    clusters = find_clusters(data)
    print("[*] кластеров-кандидатов: %d" % len(clusters))

    scored = []
    for (i, d1, d2, d3) in clusters:
        ea = start + i
        func = ida_funcs.get_func(ea)
        if func is None:
            continue
        size = func.end_ea - func.start_ea
        if size < 0x400:
            continue

        # сколько дальних jcc выше кластера в пределах 0x300 байт
        lo = max(func.start_ea - start, i - 0x300)
        above = 0
        pos = data.find(b"\x0f", lo)
        while 0 <= pos < i:
            if 0x80 <= data[pos + 1] <= 0x8F:
                above += 1
            pos = data.find(b"\x0f", pos + 1)

        scored.append((above, size, ea, func, d1, d2, d3))

    scored.sort(key=lambda r: (-r[0], r[1]))
    if not scored:
        print("[!] ничего не нашлось — пришли листинг начала функции, подберу вручную")
        return

    print("\n=== КАНДИДАТЫ ===")
    for idx, (above, size, ea, func, d1, d2, d3) in enumerate(scored[:8], 1):
        print("#%d  func_rva=%X  size=0x%X  near_jcc_above=%d  cluster_rva=%X  шаги: +0x%X +0x%X +0x%X"
              % (idx, func.start_ea - imagebase, size, above, ea - imagebase, d1, d2, d3))

    above, size, ea, func, d1, d2, d3 = scored[0]
    func_start, func_end = func.start_ea, func.end_ea

    print("\n=== ВЫБРАН КАНДИДАТ #1 ===")
    print("func_rva      = 0x%X" % (func_start - imagebase))
    print("func_size     = 0x%X" % size)
    print("cluster_rva   = 0x%X" % (ea - imagebase))
    print("cluster_off   = +0x%X (от начала функции)" % (ea - func_start))

    if func_start - imagebase == REF_FUNC_RVA and size == REF_FUNC_SIZE:
        print("\n[!] Функция совпала с эталоном 1.26.51 (RVA 0x2FB8DE0, 0xEEE байт):")
        print("    модуль из присланного кода подойдёт как есть, сигнатуры не нужны.")

    pattern_result = unique_signature(func_start, data)
    print("\n=== СИГНАТУРА НАЧАЛА ФУНКЦИИ ===")
    if pattern_result is None:
        print("[!] уникальную сигнатуру собрать не удалось — пришли листинг")
    else:
        pattern, length = pattern_result
        sig = to_libhat(pattern)
        print("длина: %d байт, вхождений в .text: 1" % length)
        print(sig)
        print("\n--- для src/SDK/SigManager.hpp ---")
        print("  DEFINE_SIG(Criticals_attack,")
        print(wrap_cpp(sig))
        print("             SigType::Sig, 0);")

    print("\n=== УСЛОВНЫЕ ПЕРЕХОДЫ ВНУТРИ ФУНКЦИИ ===")
    rows = jcc_report(func_start, func_end)
    by_target = {}
    for off, addr, raw, target in rows:
        inside = func_start <= target < func_end
        print("  +0x%-4X  %-20s -> %s%s"
              % (off, " ".join("%02X" % b for b in raw),
                 "внутри" if inside else "СНАРУЖИ",
                 "  (+0x%X)" % (target - func_start) if inside else ""))
        if inside:
            by_target.setdefault(target - func_start, []).append(off)

    print("\n  группировка по цели (самая большая группа — обычно «не крит»):")
    for target_off, sites in sorted(by_target.items(), key=lambda kv: -len(kv[1])):
        print("   +0x%-4X <- %s" % (target_off, ", ".join("+0x%X" % s for s in sites)))

    # Инвариант, на котором держится нативный патч в клиенте (CritPatchUtils):
    # общая цель «плохих» переходов == адрес сразу за последним переходом,
    # а сам последний уходит далеко вперёд, в крит-блок.
    anchor_off = ea - func_start
    anchor_fall = anchor_off + 6
    anchor_target = None
    for off, addr, raw, target in rows:
        if addr == ea:
            anchor_target = target - func_start
            break

    print("\n=== ПРОВЕРКА ИНВАРИАНТА ===")
    print("  последний jcc: +0x%X, за ним +0x%X, ведёт в +0x%X"
          % (anchor_off, anchor_fall, anchor_target if anchor_target is not None else -1))
    shared = by_target.get(anchor_fall, [])
    print("  переходов в +0x%X: %d -> %s" % (anchor_fall, len(shared),
                                             ", ".join("+0x%X" % x for x in shared)))
    if anchor_target is not None and len(shared) >= 5 and anchor_target > anchor_fall + 0x80:
        print("  ИТОГ: структура та же, что в 1.26.51 — нативный патч найдёт её сам,")
        print("        сигнатуры и RVA не нужны. Достаточно включить Criticals -> Native.")
    else:
        print("  ИТОГ: структура отличается — пришли листинг, разберём вручную.")

    print("\n=== СВЕРКА С ЭТАЛОНОМ 1.26.51 ===")
    for off, original in REF_SITES:
        current = data[func_start - start + off: func_start - start + off + len(original)]
        if len(current) != len(original):
            print("  +0x%-4X  вне функции" % off)
            continue
        mark = "совпадает" if current == original else "отличается"
        print("  +0x%-4X  было %-20s стало %-20s %s"
              % (off, " ".join("%02X" % b for b in original),
                 " ".join("%02X" % b for b in current), mark))

    print("\n[!] Не запускай патч, пока не посмотришь глазами, куда ведёт последний jcc:")
    print("    он должен вести в блок, где считается крит-урон / партиклы.")


if __name__ == "__main__":
    if HAVE_IDA:
        main()
    else:
        print("Запускать внутри IDA: File -> Script file...")
