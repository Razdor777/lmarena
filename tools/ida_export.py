#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ida_export.py — IDAPython-скрипт для экспорта RE-данных из Minecraft.Windows.exe.

ЗАЧЕМ: клиент Solstice/Moonlight хардкодит адреса (см. AmbienceHook.cpp) и использует
смещения, которые никто не проверял. Этот скрипт за один прогон в IDA выгружает всё,
что нужно, чтобы превратить дампы в рабочие хуки:

  * дерево вызовов (функция -> её callees, до MAX_DEPTH) с RVA и размером;
  * список тех, кто вызывает функцию (callers), включая xrefs из vtable/данных;
  * строки, на которые ссылается функция (это лучший маркер "что это за пасс");
  * глобальные переменные (dword_/qword_/unk_) — кандидаты в тумблеры фич;
  * смещения полей: все [reg+disp] внутри функции, которые встречаются в коде;
  * готовую сигнатуру в формате libhat (как в src/SDK/SigManager.hpp) для каждой функции;
  * декомпилированный псевдокод небольших функций (Hex-Rays), чтобы не копировать его руками.

ИСПОЛЬЗОВАНИЕ
  1) Открыть .idb/.i64 в IDA Pro (Hex-Rays желателен, но не обязателен).
  2) File -> Script file... -> выбрать tools/ida_export.py
     (или Alt+F7, или в командной строке IDA: Alt+F2 и вставить путь).
  3) Рядом со скриптом появится ida_export.json — его и надо прислать.

  Можно указать корни вручную (через пробел), тогда берётся не адрес под курсором:
     ida -A -S"tools/ida_export.py 0x14424FCD0 0x1442531D0" Minecraft.Windows.exe

НАСТРОЙКИ — ниже, в блоке CONFIG.
"""

import json
import os
import sys
import time
import traceback

# --------------------------------------------------------------------------------------
# CONFIG
# --------------------------------------------------------------------------------------
# Корни обхода. [] = взять адрес под курсором в IDA (или единственную функцию, если их мало).
ROOTS = []

# Насколько глубоко идти по callees (1 = только сама функция).
MAX_DEPTH = 3

# Максимум функций в выгрузке (защита от гигантских деревьев вроде RenderFrame).
MAX_FUNCS = 300

# Сколько байт от начала функции брать в сигнатуру.
SIG_LEN = 48

# Включать псевдокод Hex-Rays для функций короче N строк (None = не включать вовсе).
PSEUDOCODE_MAX_LINES = 400

# Псевдокод только для функций на глубине <= N (корень и его прямые callees).
PSEUDOCODE_MAX_DEPTH = 1

# Куда писать. None = рядом со скриптом.
OUT_PATH = None

# --------------------------------------------------------------------------------------
# IDA API
# --------------------------------------------------------------------------------------
try:
    import ida_auto
    import ida_bytes
    import ida_funcs
    import ida_ida
    import ida_nalt
    import ida_name
    import ida_segment
    import ida_ua
    import ida_xref
    import idautils
    import idc

    ida_auto.auto_wait()
except ImportError as exc:  # запущено не в IDA
    sys.stderr.write("This script must be run inside IDA Pro (IDAPython): %s\n" % exc)
    raise

try:
    import ida_hexrays

    HAVE_HEXRAYS = True
except ImportError:
    HAVE_HEXRAYS = False

# libhat-подобный формат: "48 8B ? ? ? ? FF 15 ? ? ? ?"
WILDCARD = "?"
IMAGE_BASE = ida_ida.inf_get_imagebase() or 0x140000000
WORD_SIZE = 8 if ida_ida.inf_is_64bit() else 4


# --------------------------------------------------------------------------------------
# ХЕЛПЕРЫ
# --------------------------------------------------------------------------------------
def safe(fn, default):
    try:
        return fn()
    except Exception:
        return default


def func_end(ea):
    end = idc.get_func_attr(ea, idc.FUNCATTR_END)
    return end if end and end != idc.BADADDR else ea + 1


def func_start(ea):
    start = idc.get_func_attr(ea, idc.FUNCATTR_START)
    return start if start and start != idc.BADADDR else None


def insn_bytes(ea, size):
    data = ida_bytes.get_bytes(ea, size)
    return data if data and len(data) == size else None


def wildcard_for_operand(ea, insn, op, index):
    """Возвращает список (offset_in_insn, length) байт, которые надо замаскировать под ?."""
    masks = []
    try:
        text = idc.print_operand(ea, index) or ""
    except Exception:
        text = ""

    # o_mem / o_near / o_far: это адрес (RIP-rel или rel32 у call/jmp) — маскируем хвост.
    if op.type in (ida_ua.o_mem, ida_ua.o_near, ida_ua.o_far):
        masks.append((insn.size - 4, 4))
    # o_displ: [reg+disp] или [rip+disp]. Маскируем только rip-относительные.
    elif op.type == ida_ua.o_displ:
        if "rip" in text.lower():
            masks.append((insn.size - 4, 4))
    # o_imm: маскируем, если это похоже на адрес (>= базы образа) — такие immediate
    # меняются между сборками и ломают сигнатуру.
    elif op.type == ida_ua.o_imm:
        # Маскируем только абсолютные адреса (они меняются между сборками).
        # Маленькие immediate (смещения полей, размеры) оставляем — они повышают уникальность.
        val = op.value & ((1 << (WORD_SIZE * 8)) - 1)
        if val >= IMAGE_BASE and 0 < op.size <= insn.size:
            masks.append((insn.size - op.size, op.size))

    out = []
    for off, length in masks:
        if off < 0 or off + length > insn.size:
            continue
        out.append((off, length))
    return out


def build_signature(ea, end, sig_len=SIG_LEN):
    """Сигнатура в формате libhat по первым байтам функции, с ? на релоцируемых операндах."""
    parts = []
    cur = ea
    while cur < end and (cur - ea) < sig_len and len(parts) < 64:
        insn = ida_ua.insn_t()
        if not ida_ua.decode_insn(insn, cur) or insn.size == 0:
            break
        raw = insn_bytes(cur, insn.size)
        if raw is None:
            break
        masked = bytearray(insn.size)
        for index, op in enumerate(insn.ops):
            for off, length in wildcard_for_operand(cur, insn, op, index):
                for i in range(off, off + length):
                    if 0 <= i < insn.size:
                        masked[i] = 1
        for i, byte in enumerate(raw):
            parts.append(WILDCARD if masked[i] else "%02X" % byte)
        cur += insn.size

    return " ".join(parts), " ".join("%02X" % b for b in (insn_bytes(ea, min(32, max(1, end - ea))) or b""))


def collect_strings(ea, end, limit=120):
    found = {}
    for item in idautils.FuncItems(ea):
        for ref in idautils.DataRefsFrom(item):
            if ref in found:
                continue
            text = idc.get_strlit_contents(ref, -1, idc.STRTYPE_C)
            if text:
                try:
                    text = text.decode("utf-8", "replace")
                except Exception:
                    text = str(text)
                found[ref] = text
                if len(found) >= limit:
                    return found
    return found


def collect_globals(ea, end, limit=120):
    """Глобальные переменные (dword_/qword_/unk_/byte_) + их текущее содержимое."""
    found = {}
    for item in idautils.FuncItems(ea):
        for ref in idautils.DataRefsFrom(item):
            if ref in found:
                continue
            seg = ida_segment.getseg(ref)
            if seg is None:
                continue
            name = ida_name.get_name(ref) or ""
            if not name:
                continue
            raw = ida_bytes.get_bytes(ref, 8) or b""
            found[ref] = {
                "name": name,
                "segment": ida_segment.get_segm_name(seg),
                "perm": int(seg.perm),
                "bytes": raw.hex().upper(),
                "value_u64": int.from_bytes(raw[:8].ljust(8, b"\x00"), "little") if raw else 0,
            }
            if len(found) >= limit:
                return found
    return found


def collect_displacements(ea, end, limit=400):
    """Все [reg+disp] с маленьким disp — это смещения полей структур."""
    found = {}
    for item in idautils.FuncItems(ea):
        insn = ida_ua.insn_t()
        if not ida_ua.decode_insn(insn, item):
            continue
        for index, op in enumerate(insn.ops):
            if op.type != ida_ua.o_displ:
                continue
            disp = op.addr & 0xFFFFFFFF
            if disp == 0 or disp >= 0x10000:
                continue
            try:
                text = idc.print_operand(item, index) or ""
            except Exception:
                text = ""
            key = (disp, text)
            if key not in found:
                found[key] = {
                    "disp": disp,
                    "hex": "0x%X" % disp,
                    "operand": text,
                    "line": idc.GetDisasm(item) or "",
                }
    ordered = sorted(found.values(), key=lambda d: d["disp"])
    return ordered[:limit]


def collect_calls(ea, end):
    """Прямые call'ы внутри функции: (адрес вызова, цель, имя цели)."""
    calls = {}
    for item in idautils.FuncItems(ea):
        try:
            mnem = idc.print_insn_mnem(item) or ""
        except Exception:
            continue
        if not mnem.startswith("call"):
            continue
        for target in idautils.CodeRefsFrom(item, 0):
            start = func_start(target)
            if start is None:
                continue
            if start in calls:
                continue
            calls[start] = {
                "target": start,
                "rva": start - IMAGE_BASE,
                "name": idc.get_func_name(start),
                "call_site": item,
            }
    return calls


def collect_callers(ea, limit=60):
    out = []
    seen = set()
    for item in idautils.XrefsTo(ea, 0):
        if item.frm in seen:
            continue
        seen.add(item.frm)
        out.append({
            "from": item.frm,
            "rva": item.frm - IMAGE_BASE,
            "type": int(item.type),
            "func": func_start(item.frm),
            "func_name": idc.get_func_name(func_start(item.frm)) if func_start(item.frm) else "",
            "line": idc.GetDisasm(item.frm) or "",
        })
        if len(out) >= limit:
            break
    direct = [x for x in out if x["func_name"]]
    return {"all_count": len(out), "direct": direct, "other": [x for x in out if not x["func_name"]]}


def pseudocode(ea):
    if not HAVE_HEXRAYS or not PSEUDOCODE_MAX_LINES:
        return None
    try:
        cfunc = ida_hexrays.decompile(ea)
    except Exception:
        return None
    if not cfunc:
        return None
    text = str(cfunc)
    lines = text.splitlines()
    if len(lines) > PSEUDOCODE_MAX_LINES:
        return {"lines": len(lines), "truncated": True, "text": "\n".join(lines[:PSEUDOCODE_MAX_LINES])}
    return {"lines": len(lines), "truncated": False, "text": text}


def safe_demangle(ea):
    try:
        flags = idc.get_inf_attr(idc.INF_SHORT_DN) if hasattr(idc, "INF_SHORT_DN") else 0
    except Exception:
        flags = 0
    try:
        return idc.demangle_name(idc.get_name(ea) or "", flags) or ""
    except Exception:
        return ""


def describe_function(ea, depth=0):
    start = func_start(ea) or ea
    end = func_end(start)
    entry = {
        "ea": start,
        "rva": start - IMAGE_BASE,
        "name": idc.get_func_name(start),
        "demangled": safe_demangle(start),
        "size": int(end - start),
        "segment": idc.get_segm_name(start),
        "flags": int(idc.get_func_attr(start, idc.FUNCATTR_FLAGS) or 0),
        "is_lib": bool(idc.get_func_attr(start, idc.FUNCATTR_FLAGS) & idc.FUNC_LIB) if hasattr(idc, "FUNC_LIB") else False,
        "callers": collect_callers(start),
        "calls": {},
        "strings": {},
        "globals": {},
        "displacements": [],
    }

    try:
        entry["signature"], entry["prologue_hex"] = build_signature(start, end)
    except Exception:
        entry["signature"], entry["prologue_hex"] = "", ""
        entry["signature_error"] = traceback.format_exc(limit=1)

    for key, fn in (("calls", collect_calls), ("strings", collect_strings),
                    ("globals", collect_globals), ("displacements", collect_displacements)):
        try:
            entry[key] = fn(start, end)
        except Exception:
            entry[key] = {} if key != "displacements" else []
            entry[key + "_error"] = traceback.format_exc(limit=1)

    if isinstance(entry["calls"], dict):
        entry["calls"] = {
            "count": len(entry["calls"]),
            "items": sorted(entry["calls"].values(), key=lambda c: c["target"]),
        }
    if isinstance(entry["strings"], dict):
        entry["strings"] = {("%X" % k): v for k, v in entry["strings"].items()}
    if isinstance(entry["globals"], dict):
        entry["globals"] = {("%X" % k): v for k, v in entry["globals"].items()}

    entry["pseudocode"] = pseudocode(start) if depth <= PSEUDOCODE_MAX_DEPTH else None
    return entry


# --------------------------------------------------------------------------------------
# MAIN
# --------------------------------------------------------------------------------------
def main():
    roots = []

    for arg in sys.argv[1:]:
        try:
            roots.append(int(arg, 0))
        except ValueError:
            pass

    if not roots:
        roots = list(ROOTS)

    if not roots:
        try:
            screen = idc.get_screen_ea()
        except Exception:
            screen = idc.here() if hasattr(idc, "here") else 0
        start = func_start(screen) or screen
        if start:
            roots.append(start)

    if not roots:
        sys.stderr.write("ida_export: no roots — put the cursor inside a function or pass addresses\n")
        return

    result = {
        "meta": {
            "tool": "tools/ida_export.py",
            "generated_at": time.strftime("%Y-%m-%d %H:%M:%S"),
            "input_file": safe(lambda: ida_nalt.get_root_filename() or ida_nalt.get_input_file_path(), ""),
            "input_md5": safe(lambda: ida_nalt.retrieve_input_file_md5().hex(), ""),
            "idb_path": safe(lambda: idc.get_idb_path(), ""),
            "image_base": IMAGE_BASE,
            "word_size": WORD_SIZE,
            "ida_version": safe(ida_ida.inf_get_version, ""),
            "has_hexrays": HAVE_HEXRAYS,
        },
        "roots": roots,
        "functions": [],
    }

    queue = [(root, 0) for root in roots]
    seen = {}
    while queue and len(seen) < MAX_FUNCS:
        ea, depth = queue.pop(0)
        start = func_start(ea) or ea
        if start in seen:
            continue
        try:
            entry = describe_function(start, depth)
        except Exception:
            sys.stderr.write("ida_export: failed on %X\n%s\n" % (start, traceback.format_exc()))
            continue
        entry["depth"] = depth
        seen[start] = entry
        if depth + 1 <= MAX_DEPTH:
            for call in entry["calls"].get("items", []):
                if call["target"] not in seen:
                    queue.append((call["target"], depth + 1))

    result["functions"] = sorted(seen.values(), key=lambda f: (f["depth"], f["ea"]))
    result["meta"]["function_count"] = len(result["functions"])

    try:
        base_dir = os.path.dirname(os.path.abspath(__file__))
    except NameError:
        base_dir = os.getcwd()
    out = OUT_PATH or os.path.join(base_dir, "ida_export.json")
    with open(out, "w", encoding="utf-8") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=1)

    size_kb = os.path.getsize(out) / 1024.0
    print("ida_export: %d functions -> %s (%.1f KB)" % (len(result["functions"]), out, size_kb))


main()
