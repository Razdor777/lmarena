#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
sig_migrate.py — перенос клиента (Solstice) с одной версии Minecraft на другую.

ЗАЧЕМ
  Весь доступ к игре держится на ~230 записях:
    * src/SDK/SigManager.hpp      — 124 сигнатуры (DEFINE_SIG)
    * src/SDK/OffsetProvider.hpp  — 63 DEFINE_INDEX_FIELD / DEFINE_INDEX_FIELD_TYPED + 45 DEFINE_FIELD
    * src/Hook/Hooks/RenderHooks/AmbienceHook.cpp — 6 жёстких адресов
  При смене версии игры каждая такая запись может «поехать». Этот скрипт берёт
  СВЕЖИЙ exe, прогоняет по нему все сигнатуры из репозитория и говорит по каждой:
  нашлась ровно одна (адрес готов), нашлась несколько (нужно уточнить), не нашлась
  (нужен ручной разбор — и тут помогает --names из PDB).

  То есть: «обновить клиент до версии X» = прогнать этот скрипт по exe версии X,
  разобрать отчёт, добить оставшиеся записи по символам (tools/pdb_dump.py --names)
  и собрать проект.

ИСПОЛЬЗОВАНИЕ
  :: 1) что вообще распознаётся из репозитория
  python3 tools/sig_migrate.py --parse

  :: 2) миграция на новый exe
  python3 tools/sig_migrate.py --exe "C:\\...\\Minecraft.Windows.exe" --md migration.md

  :: 3) то же, но с символами (подсказки для ненайденных записей)
  python3 tools/sig_migrate.py --exe new.exe --names names.txt --md migration.md

  names.txt получается так: python3 tools/pdb_dump.py client.pdb --names names.txt

ВАЖНО
  * Скрипт НИЧЕГО не меняет в репозитории — он только читает и выдаёт отчёт.
  * Сигнатура, которая нашлась ровно один раз в новом exe, остаётся валидной: править
    её не нужно, меняется только база (она и так вычисляется по сигнатуре).
  * Записи DEFINE_FIELD (числовые смещения структур) сигнатурами не проверяются:
    они помечаются как MANUAL и разбираются по символьным типам (PDB).
"""

from __future__ import annotations

import argparse
import difflib
import json
import os
import re
import struct
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SIGMANAGER = os.path.join(REPO, "src", "SDK", "SigManager.hpp")
OFFSETPROVIDER = os.path.join(REPO, "src", "SDK", "OffsetProvider.hpp")
AMBIENCEHOOK = os.path.join(REPO, "src", "Hook", "Hooks", "RenderHooks", "AmbienceHook.cpp")


def set_repo(root: str):
    """Позволяет разобрать другой корень (--repo)."""
    global REPO, SIGMANAGER, OFFSETPROVIDER, AMBIENCEHOOK
    REPO = os.path.abspath(root)
    SIGMANAGER = os.path.join(REPO, "src", "SDK", "SigManager.hpp")
    OFFSETPROVIDER = os.path.join(REPO, "src", "SDK", "OffsetProvider.hpp")
    AMBIENCEHOOK = os.path.join(REPO, "src", "Hook", "Hooks", "RenderHooks", "AmbienceHook.cpp")


def strip_comments(text: str) -> str:
    """Убираем // , /* */ и сами макроопределения — иначе #define DEFINE_SIG(name,...)
    попадёт в разбор как настоящая запись с именем 'name'."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    # строки #define с продолжениями через "\"
    out_lines = []
    skipping = False
    for line in text.splitlines():
        if skipping:
            skipping = line.rstrip().endswith("\\")
            continue
        if re.match(r"\s*#\s*define\b", line):
            skipping = line.rstrip().endswith("\\")
            continue
        out_lines.append(line)
    return "\n".join(out_lines)

STATUS_UNIQUE = "OK_UNIQUE"
STATUS_MULTI = "MULTIPLE"
STATUS_MISS = "NOT_FOUND"
STATUS_MANUAL = "MANUAL"
STATUS_BAD = "BAD_PATTERN"

# --------------------------------------------------------------------------------------
# Разбор репозитория
# --------------------------------------------------------------------------------------
STRING_LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')


def join_string_literals(text: str) -> str:
    """'48 8B ? ?' ' FF 15' -> '48 8B ? ? FF 15'"""
    parts = STRING_LITERAL.findall(text)
    return "".join(part.replace('\\"', '"') for part in parts)


def parse_sigmanager(path: str):
    """DEFINE_SIG(имя, "паттерн"[, ...], SigType::X, offset)"""
    entries = []
    if not os.path.exists(path):
        return entries
    text = strip_comments(open(path, encoding="utf-8", errors="replace").read())
    for match in re.finditer(r"DEFINE_SIG\s*\(", text):
        start = match.end()
        # вырезаем аргументы до сбалансированной закрывающей скобки
        depth = 1
        pos = start
        in_string = False
        while pos < len(text) and depth:
            char = text[pos]
            if char == '"' and text[pos - 1] != "\\":
                in_string = not in_string
            elif not in_string:
                if char == "(":
                    depth += 1
                elif char == ")":
                    depth -= 1
                    if depth == 0:
                        break
            pos += 1
        args = text[start:pos]
        name_match = re.match(r"\s*([A-Za-z_]\w*)", args)
        if not name_match:
            continue
        name = name_match.group(1)
        rest = args[name_match.end():]
        pattern = join_string_literals(rest)
        sig_type_match = re.search(r"SigType::(\w+)", rest)
        offset_match = re.findall(r",\s*(\d+|0x[0-9A-Fa-f]+)\s*\)?\s*$", rest)
        entries.append({
            "file": os.path.relpath(path, REPO),
            "kind": "DEFINE_SIG",
            "name": name,
            "pattern": pattern,
            "sig_type": sig_type_match.group(1) if sig_type_match else "Sig",
            "offset": int(offset_match[0], 0) if offset_match else 0,
        })
    return entries


def parse_offsetprovider(path: str):
    """DEFINE_INDEX_FIELD(_TYPED) и DEFINE_FIELD."""
    entries = []
    if not os.path.exists(path):
        return entries
    text = strip_comments(open(path, encoding="utf-8", errors="replace").read())

    typed = re.compile(r"DEFINE_INDEX_FIELD_TYPED\s*\(\s*[\w:]+\s*,\s*(\w+)\s*,")
    plain = re.compile(r"DEFINE_INDEX_FIELD\s*\(\s*(\w+)\s*,")
    for regex, kind in ((typed, "DEFINE_INDEX_FIELD_TYPED"), (plain, "DEFINE_INDEX_FIELD")):
        for match in regex.finditer(text):
            name = match.group(1)
            start = match.end()
            depth = 1
            pos = start
            in_string = False
            while pos < len(text) and depth:
                char = text[pos]
                if char == '"' and text[pos - 1] != "\\":
                    in_string = not in_string
                elif not in_string:
                    if char == "(":
                        depth += 1
                    elif char == ")":
                        depth -= 1
                        if depth == 0:
                            break
                pos += 1
            args = text[start:pos]
            pattern = join_string_literals(args)
            offset_type = re.search(r"OffsetType::(\w+)", args)
            numbers = re.findall(r",\s*(\d+|0x[0-9A-Fa-f]+)\s*,", args)
            entries.append({
                "file": os.path.relpath(path, REPO),
                "kind": kind,
                "name": name,
                "pattern": pattern,
                "offset_type": offset_type.group(1) if offset_type else "Index",
                "index_offset": int(numbers[-1], 0) if numbers else 0,
            })

    for match in re.finditer(r"DEFINE_FIELD\s*\(\s*(\w+)\s*,\s*([^)]+)\)", text):
        entries.append({
            "file": os.path.relpath(path, REPO),
            "kind": "DEFINE_FIELD",
            "name": match.group(1),
            "value": match.group(2).strip(),
        })
    return entries


def parse_hardcoded_addresses(path: str):
    """Жёсткие адреса вида 0x14XXXXXXXX внутри namespace AmbienceAddresses."""
    entries = []
    if not os.path.exists(path):
        return entries
    text = strip_comments(open(path, encoding="utf-8", errors="replace").read())
    block = re.search(r"namespace\s+AmbienceAddresses\s*\{(.*?)\}", text, re.S)
    if not block:
        return entries
    for match in re.finditer(r"constexpr\s+uintptr_t\s+(\w+)\s*=\s*(0x14[0-9A-Fa-f]{7})", block.group(1)):
        if match.group(1).upper().endswith("IMAGE_BASE") or int(match.group(2), 16) == 0x140000000:
            continue  # это сама база образа, а не адрес функции
        entries.append({
            "file": os.path.relpath(path, REPO),
            "kind": "HARDCODED",
            "name": match.group(1),
            "address": match.group(2),
            "rva": int(match.group(2), 16) - 0x140000000,
        })
    return entries


# --------------------------------------------------------------------------------------
# Паттерн -> байты + маска
# --------------------------------------------------------------------------------------
def pattern_to_bytes(pattern: str):
    tokens = [t for t in re.split(r"[\s,]+", pattern.strip()) if t]
    if not tokens:
        return None, None
    data = bytearray()
    mask = bytearray()
    for token in tokens:
        token = token.strip()
        if token in ("?", "??", "**"):
            data.append(0)
            mask.append(0)
            continue
        if len(token) == 2 and all(c in "0123456789abcdefABCDEF" for c in token):
            data.append(int(token, 16))
            mask.append(0xFF)
            continue
        if len(token) == 2 and (token[0] in "0123456789abcdefABCDEF?" and token[1] in "0123456789abcdefABCDEF?"):
            # полубайтовый wildcard вида "4?" — считаем байт обязательным, сравнение хитрее
            hi, lo = token[0], token[1]
            data.append((int(hi, 16) << 4) if hi != "?" else 0)
            mask.append(0xF0 if hi != "?" and lo == "?" else (0x0F if hi == "?" else 0xFF))
            if hi == "?" and lo == "?":
                mask[-1] = 0x00
            continue
        return None, None
    if not any(mask):
        return None, None
    return bytes(data), bytes(mask)


def find_pattern(section_data: bytes, base_rva: int, data: bytes, mask: bytes, limit: int = 64):
    """Возвращает список RVA найденных совпадений."""
    results = []
    fixed_runs = []
    index = 0
    while index < len(mask):
        if mask[index] == 0xFF or mask[index] == 0xF0 or mask[index] == 0x0F:
            start = index
            while index < len(mask) and mask[index] != 0:
                index += 1
            fixed_runs.append((start, index - start))
        else:
            index += 1
    # самый длинный непрерывный «жёсткий» участок — якорь для поиска
    runs = [(s, l) for s, l in fixed_runs if l >= 4]
    if runs:
        anchor_offset, _anchor_len = max(runs, key=lambda r: r[1])
        anchor = data[anchor_offset:anchor_offset + next(l for s, l in runs if s == anchor_offset)]
    else:
        anchor_offset, anchor = 0, bytes(
            data[i] for i in range(len(data)) if mask[i] in (0xFF, 0xF0, 0x0F))
    if not anchor:
        return results

    search_from = 0
    while len(results) < limit:
        found = section_data.find(anchor, search_from)
        if found < 0:
            break
        candidate = found - anchor_offset
        search_from = found + 1
        if candidate < 0 or candidate + len(mask) > len(section_data):
            continue
        ok = True
        for i, m in enumerate(mask):
            if m == 0:
                continue
            value = section_data[candidate + i]
            if m == 0xFF:
                if value != data[i]:
                    ok = False
                    break
            elif m == 0xF0:
                if (value & 0xF0) != (data[i] & 0xF0):
                    ok = False
                    break
            elif m == 0x0F:
                if (value & 0x0F) != (data[i] & 0x0F):
                    ok = False
                    break
        if ok:
            results.append(base_rva + candidate)
    return results


# --------------------------------------------------------------------------------------
# Имена символов (для подсказок)
# --------------------------------------------------------------------------------------
def load_names(path: str):
    """names.txt из tools/pdb_dump.py --names: 'RVA имя' (RVA hex без 0x)."""
    names = []
    if not path or not os.path.exists(path):
        return names
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.strip()
        if not line:
            continue
        parts = line.split(None, 1)
        if len(parts) != 2:
            continue
        try:
            rva = int(parts[0], 16)
        except ValueError:
            continue
        names.append({"rva": rva, "name": parts[1]})
    return names


def decompile_symbol_name(name: str) -> str:
    """Грубо вытаскиваем «человеческое» имя из декорированного C++ символа."""
    if name.startswith("?"):
        body = name[1:]
        head = re.split(r"@@|@", body)[0]
        return head
    return name


def tokens_of(name: str):
    text = decompile_symbol_name(name)
    text = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", text)
    return [t.lower() for t in re.split(r"[^A-Za-z0-9]+", text) if len(t) > 2]


MIN_SUGGESTION_SCORE = 0.9


def suggest_names(entry_name: str, names, limit: int = 3):
    want = tokens_of(entry_name)
    if not want:
        return []
    scored = []
    for item in names:
        have = tokens_of(item["name"])
        if not have:
            continue
        common = set(want) & set(have)
        if not common:
            continue
        score = len(common) / max(len(set(want)), 1) + difflib.SequenceMatcher(
            None, " ".join(want), " ".join(have)).ratio()
        scored.append((score, item))
    scored = [pair for pair in scored if pair[0] >= MIN_SUGGESTION_SCORE]
    scored.sort(key=lambda pair: pair[0], reverse=True)
    return [item for _score, item in scored[:limit]]


# --------------------------------------------------------------------------------------
# Основная логика
# --------------------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="Перенос сигнатур/оффсетов Solstice на новую версию игры")
    parser.add_argument("--parse", action="store_true", help="показать, что распознано из репозитория")
    parser.add_argument("--exe", help="новый Minecraft.Windows.exe (целевая версия)")
    parser.add_argument("--names", help="names.txt из tools/pdb_dump.py --names (символы целевой версии)")
    parser.add_argument("--md", help="отчёт в Markdown")
    parser.add_argument("--json", help="отчёт в JSON")
    parser.add_argument("--limit", type=int, default=30, help="сколько строк печатать в консоль")
    parser.add_argument("--repo", help="корень проекта (по умолчанию — корень репозитория этого файла)")
    args = parser.parse_args()

    if args.repo:
        set_repo(args.repo)

    entries = (parse_sigmanager(SIGMANAGER) + parse_offsetprovider(OFFSETPROVIDER)
               + parse_hardcoded_addresses(AMBIENCEHOOK))
    by_kind = {}
    for entry in entries:
        by_kind.setdefault(entry["kind"], []).append(entry)

    print("Разобрано из репозитория: %d записей" % len(entries))
    for kind, items in sorted(by_kind.items()):
        print("  %-28s %d" % (kind, len(items)))
    bad = [e for e in entries if e.get("pattern") and pattern_to_bytes(e["pattern"])[0] is None]
    if bad:
        print("  записей с нераспознанным паттерном: %d" % len(bad))

    if args.parse or not args.exe:
        print("\n(укажи --exe <новый Minecraft.Windows.exe>, чтобы прогнать миграцию)")
        for entry in entries[: args.limit]:
            preview = entry.get("pattern") or entry.get("value") or entry.get("address") or ""
            print("  %-28s %s" % (entry["name"], str(preview)[:64]))
        return 0

    # --- сканирование нового exe -----------------------------------------------------
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from pdb_fetch import PE
    pe = PE(args.exe)
    print("\nЦелевой exe: %s" % pe.describe())

    sections = []
    for section in pe.sections:
        executable = section["name"] in (".text", "PAGE", ".code") or section["raw_size"] > 0
        if not executable:
            continue
        data = pe.data[section["raw_ptr"]:section["raw_ptr"] + section["raw_size"]]
        if data:
            sections.append((section["virtual_address"], data))
    print("Секций для поиска: %s" % ", ".join(
        "%s(0x%X, %d КБ)" % (s["name"], s["virtual_address"], s["raw_size"] // 1024)
        for s in pe.sections if s["raw_size"]))

    names = load_names(args.names)
    if names:
        print("Символов для подсказок: %d" % len(names))

    report = []
    for entry in entries:
        if entry["kind"] == "DEFINE_FIELD":
            report.append({**entry, "status": STATUS_MANUAL,
                           "note": "числовое смещение структуры — проверяется по типам из PDB"})
            continue
        if entry["kind"] == "HARDCODED":
            rva = entry["rva"]
            hit = any(base <= rva < base + len(data) for base, data in sections)
            report.append({**entry, "status": STATUS_UNIQUE if hit else STATUS_MISS,
                           "rvas": [rva] if hit else [],
                           "note": "жёсткий адрес: заменить на сигнатуру/смещение от базы"})
            continue

        data, mask = pattern_to_bytes(entry["pattern"])
        if data is None:
            report.append({**entry, "status": STATUS_BAD, "note": "паттерн не распознан"})
            continue
        found = []
        for base_rva, section_data in sections:
            found += find_pattern(section_data, base_rva, data, mask)
        found = sorted(set(found))
        if len(found) == 1:
            status, note = STATUS_UNIQUE, ""
        elif len(found) > 1:
            status, note = STATUS_MULTI, "найдено %d совпадений — уточнить вручную" % len(found)
        else:
            status, note = STATUS_MISS, "паттерн не найден — код изменился"
        entry_report = {**entry, "status": status, "rvas": found, "note": note}
        if status in (STATUS_MISS, STATUS_MULTI) and names:
            entry_report["suggestions"] = [
                {"rva": item["rva"], "name": item["name"]} for item in suggest_names(entry["name"], names)
            ]
        report.append(entry_report)

    # --- сводка ---------------------------------------------------------------------
    counts = {}
    for item in report:
        counts[item["status"]] = counts.get(item["status"], 0) + 1
    print("\n=== ИТОГ ===")
    print("  готово (нашлось однозначно): %d" % counts.get(STATUS_UNIQUE, 0))
    print("  неоднозначно (несколько):    %d" % counts.get(STATUS_MULTI, 0))
    print("  не найдено (ручной разбор):  %d" % counts.get(STATUS_MISS, 0))
    print("  вручную (смещения полей):    %d" % counts.get(STATUS_MANUAL, 0))
    if counts.get(STATUS_BAD):
        print("  битые паттерны:              %d" % counts[STATUS_BAD])

    problems = [item for item in report if item["status"] in (STATUS_MISS, STATUS_MULTI)]
    if problems:
        print("\nТребуют внимания (%d):" % len(problems))
        for item in problems[: args.limit]:
            line = "  [%s] %-30s %s" % (item["status"], item["name"], item["note"])
            print(line)
            for suggestion in item.get("suggestions", []):
                print("        кандидат: 0x%X  %s" % (suggestion["rva"], suggestion["name"]))
        if len(problems) > args.limit:
            print("  ... ещё %d (полный список — в --md/--json)" % (len(problems) - args.limit))

    if args.json:
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump({"exe": args.exe, "counts": counts, "entries": report}, handle,
                      ensure_ascii=False, indent=1)
        print("\nJSON: %s" % args.json)

    if args.md:
        lines = ["# Отчёт миграции сигнатур", "",
                 "Целевой файл: `%s`" % args.exe,
                 "Разобрано записей: %d" % len(report), ""]
        lines += ["| статус | сколько |", "|---|---|"]
        for status, count in sorted(counts.items()):
            lines.append("| %s | %d |" % (status, count))
        lines += ["", "## Требуют внимания", "",
                  "| статус | имя | файл | адреса | что делать |", "|---|---|---|---|---|"]
        for item in problems:
            rvas = ", ".join("0x%X" % rva for rva in item["rvas"][:6])
            hints = "; ".join("0x%X %s" % (s["rva"], s["name"]) for s in item.get("suggestions", []))
            lines.append("| %s | %s | %s | %s | %s |" % (
                item["status"], item["name"], item.get("file", ""), rvas,
                (hints or item.get("note", ""))))
        lines += ["", "## Готовые записи (адреса в целевой версии)", "",
                  "| имя | файл | RVA |", "|---|---|---|"]
        for item in report:
            if item["status"] == STATUS_UNIQUE and item.get("rvas"):
                lines.append("| %s | %s | 0x%X |" % (item["name"], item.get("file", ""), item["rvas"][0]))
        with open(args.md, "w", encoding="utf-8") as handle:
            handle.write("\n".join(lines) + "\n")
        print("Markdown: %s" % args.md)

    return 0


if __name__ == "__main__":
    sys.exit(main())
