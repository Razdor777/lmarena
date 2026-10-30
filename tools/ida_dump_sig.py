# -*- coding: utf-8 -*-
"""
ida_dump_sig.py — снять НАСТОЯЩУЮ сигнатуру с оригинала и потом найти её в новой базе.

Работает в двух режимах (переключается константой MODE ниже).

  MODE = "dump"  — в базе ОРИГИНАЛА (1.26.51), где известен RVA функции удара.
                   Сверяет 8 известных байтовых последовательностей, печатает
                   точный дамп региона и готовые строки для
                   Search -> Sequence of bytes (с ?? на относительных операндах).

  MODE = "here"  — в базе НОВОЙ сборки (1.26.10): ставишь курсор на начало
                   найденной функции и получаешь такие же сигнатуры уже для неё
                   (пригодится, чтобы прибить её гвоздями в клиенте).

Ничего не патчит.

Что печатается в режиме dump:
  1. функция: RVA, размер, имя;
  2. сверка 8 сайтов с эталоном 1.26.51 (совпало / отличается);
  3. авторазметка цепочки: какой переход куда ведёт;
  4. HEX региона (копируется целиком в чат, если что-то пошло не так);
  5. PATTERN_CLUSTER  — от первого сайта до последнего, все относительные
     операнды заменены на ??; это и есть «сигнатура из оригинала»;
  6. PATTERN_CRIT    — кусок крит-блока (если голова функции в новой сборке
     переписана, хвост обычно переживает пересборку);
  7. SIGNATURE_START — сигнатура от начала функции по инструкциям, уникальная
     в .text (в формате libhat, как в SigManager.hpp).
"""

import re

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
# НАСТРОЙКИ
# ─────────────────────────────────────────────────────────────────────────────

MODE = "dump"          # "dump" — база оригинала; "here" — новая база, курсор на функции

# Только для MODE == "dump": смещение функции удара в оригинале.
FUNC_RVA = 0x2FB8DE0

# Оригинальные байты сайтов (из рабочего модуля под 1.26.51).
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

REF_FUNC_SIZE = 0xEEE     # для сверки «та функция или не та»
CRIT_OFFSET   = 0x712     # куда ведёт последний переход
HEX_LEN       = 0x400     # сколько байт от начала функции печатать
PATTERN_LEN   = 0x160     # сколько байт от крит-блока печатать паттерном


# ─────────────────────────────────────────────────────────────────────────────
# Байты и паттерны
# ─────────────────────────────────────────────────────────────────────────────

def operand_ranges(ea):
    """Смещения байтов инструкции, которые зависят от адреса (rel32, imm)."""
    insn = ida_ua.insn_t()
    if ida_ua.decode_insn(insn, ea) <= 0:
        return None, 0

    wild = set()
    for op in insn.ops:
        if op.type in (ida_ua.o_near, ida_ua.o_far, ida_ua.o_mem, ida_ua.o_imm):
            wild.update(range(op.offb, insn.size))

    return wild, insn.size


def pattern_of(ea_from, ea_to, wildcard_operands=True):
    """
    Паттерн для IDA: список пар (b, is_wild). Операнды-адреса — ??, остальное
    точные байты из базы. Такой паттерн не ломается от пересборки по адресам,
    но точно описывает код.
    """
    out = []
    ea = ea_from
    while ea < ea_to:
        wild, size = operand_ranges(ea)
        if wild is None:
            return None
        raw = ida_bytes.get_bytes(ea, size)
        if raw is None or len(raw) != size:
            return None
        for k in range(size):
            out.append((raw[k], wildcard_operands and k in wild))
        ea += size
    return out


def to_ida_string(pattern):
    return " ".join("??" if w else "%02X" % b for b, w in pattern)


def to_libhat_string(pattern):
    return " ".join("?" if w else "%02X" % b for b, w in pattern)


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
# Разметка цепочки
# ─────────────────────────────────────────────────────────────────────────────

def is_near_jcc(raw):
    return len(raw) >= 2 and raw[0] == 0x0F and 0x80 <= raw[1] <= 0x8F


def is_short_jcc(raw):
    return len(raw) >= 1 and 0x70 <= raw[0] <= 0x7F


def jcc_target(ea, raw):
    if is_short_jcc(raw) and len(raw) >= 2:
        disp = raw[1] - 0x100 if raw[1] >= 0x80 else raw[1]
        return ea + 2 + disp
    if is_near_jcc(raw) and len(raw) >= 6:
        disp = int.from_bytes(raw[2:6], "little", signed=True)
        return ea + 6 + disp
    return None


def describe_chain(func_start, func_end, base):
    rows = []
    ea = func_start
    while ea < func_end:
        insn = ida_ua.insn_t()
        if ida_ua.decode_insn(insn, ea) <= 0:
            break
        raw = ida_bytes.get_bytes(ea, insn.size) or b""
        target = jcc_target(ea, raw)
        if target is not None:
            rows.append((ea, raw, target))
        ea += insn.size
    return rows


def autodetect(hits, base, func_end):
    """Сайты = переходы в общую цель, где цель = байт за последним переходом."""
    for ea, raw, target in hits:
        fall = ea + len(raw)
        group = [h for h in hits if h[2] == fall]
        if len(group) >= 5 and target > fall + 0x80:
            return group, fall, target
    return None, None, None


# ─────────────────────────────────────────────────────────────────────────────

def dump(func_start):
    base = idaapi.get_imagebase()
    func = ida_funcs.get_func(func_start)
    func_end = func.end_ea if func else func_start + HEX_LEN
    size = func_end - func_start

    print("\n=== ФУНКЦИЯ ===")
    print("  start_rva = 0x%X   end_rva = 0x%X   size = 0x%X   name = %s"
          % (func_start - base, func_end - base, size, idc.get_func_name(func_start)))
    if MODE == "dump":
        same = (func_start - base == FUNC_RVA and size == REF_FUNC_SIZE)
        print("  сверка с оригиналом: rva %s, size %s"
              % ("совпал" if func_start - base == FUNC_RVA else "ДРУГОЙ",
                 "совпал" if size == REF_FUNC_SIZE else "ДРУГОЙ"))

    # 1) сверка известных сайтов
    print("\n=== САЙТЫ ===")
    for off, original in REF_SITES:
        ea = func_start + off
        current = ida_bytes.get_bytes(ea, len(original)) or b""
        if len(current) != len(original):
            print("  +0x%-4X вне функции" % off)
            continue
        print("  +0x%-4X  было %-20s сейчас %-20s %s"
              % (off,
                 " ".join("%02X" % b for b in original),
                 " ".join("%02X" % b for b in current),
                 "OK" if current == original else "ОТЛИЧАЕТСЯ"))

    # 2) авторазметка
    print("\n=== ЦЕПОЧКА ===")
    hits = describe_chain(func_start, func_end, base)
    group, fall, crit = autodetect(hits, base, func_end)
    if group is None:
        print("  цепочка не распознана (в MODE=here это нормально — печатаем как есть)")
    else:
        print("  «не крит» = +0x%X, крит = +0x%X" % (fall - func_start, crit - func_start))
        for ea, raw, target in group:
            print("  +0x%-4X  %-20s -> +0x%X"
                  % (ea - func_start, " ".join("%02X" % b for b in raw),
                     (target - func_start) if target >= func_start else -1))

    # 3) hex
    print("\n=== HEX (0x%X байт от начала функции) ===" % HEX_LEN)
    raw = ida_bytes.get_bytes(func_start, HEX_LEN) or b""
    for row in range(0, len(raw), 16):
        print("  %04X: %s" % (row, " ".join("%02X" % b for b in raw[row:row + 16])))

    # 4) паттерн кластера (сайты известны в режиме dump, иначе — найденная группа)
    print("\n=== PATTERN_CLUSTER (Search -> Sequence of bytes) ===")
    if group is not None:
        first = group[0][0]
        last = group[-1][0] + len(group[-1][1])
        pattern = pattern_of(first, last)
        if pattern is None:
            print("  не удалось построить паттерн")
        else:
            text = to_ida_string(pattern)
            print("  от +0x%X до +0x%X, %d байт" % (first - func_start, last - func_start, len(pattern)))
            print("  " + text)
            print("\n  то же в формате libhat (для SigManager.hpp):")
            print(wrap_cpp(to_libhat_string(pattern)))

    # 5) крит-блок
    print("\n=== PATTERN_CRIT (хвост, живучий при пересборке) ===")
    crit_ea = func_start + CRIT_OFFSET
    pattern = pattern_of(crit_ea, crit_ea + PATTERN_LEN)
    if pattern is None:
        print("  крит-блок вне функции — смещение 0x%X не подходит" % CRIT_OFFSET)
    else:
        print("  от +0x%X, %d байт" % (CRIT_OFFSET, len(pattern)))
        print("  " + to_ida_string(pattern))

    # 6) сигнатура от начала функции, уникальная в .text
    print("\n=== SIGNATURE_START (уникальная в .text) ===")
    seg = ida_segment.get_segm_by_name(".text")
    data = ida_bytes.get_bytes(seg.start_ea, seg.end_ea - seg.start_ea) if seg else None
    pattern = pattern_of(func_start, func_start + 0x40)
    if pattern is None or data is None:
        print("  не удалось")
    else:
        for grow in (0x40, 0x60, 0x80, 0xA0, 0xC0):
            candidate = pattern_of(func_start, func_start + grow)
            if candidate is None:
                continue
            text = to_libhat_string(candidate)
            # грубая проверка уникальности: по самому длинному непрерывному
            # куску без wildcard
            best, run, start = (0, 0), 0, 0
            for idx, (b, w) in enumerate(candidate):
                if w:
                    if run > best[0]:
                        best = (run, start)
                    run = 0
                else:
                    if run == 0:
                        start = idx
                    run += 1
            if run > best[0]:
                best = (run, start)
            if best[0] < 4:
                continue
            anchor = bytes(b for b, w in candidate[best[1]:best[1] + best[0]])
            count = 0
            pos = data.find(anchor)
            while pos >= 0:
                count += 1
                pos = data.find(anchor, pos + 1)
            if count == 1:
                print("  длина %d байт, вхождений 1:" % (grow))
                print("  " + text)
                print("\n  для src/SDK/SigManager.hpp:")
                print("  DEFINE_SIG(Criticals_attack,")
                print(wrap_cpp(text))
                print("               SigType::Sig, 0);")
                break
        else:
            print("  уникальность не подтвердилась до 0xC0 байт — используй PATTERN_CLUSTER")


def main():
    if MODE == "dump":
        start = idaapi.get_imagebase() + FUNC_RVA
        print("[*] MODE=dump, FUNC_RVA=0x%X -> %X" % (FUNC_RVA, start))
    else:
        start = idc.here()
        print("[*] MODE=here, курсор на %X" % start)

    func = ida_funcs.get_func(start)
    if func is None:
        print("[!] по этому адресу IDA не видит функцию — встань на её первый байт")
        return

    dump(func.start_ea)


if __name__ == "__main__":
    if HAVE_IDA:
        main()
    else:
        print("Запускать внутри IDA: File -> Script file...")
