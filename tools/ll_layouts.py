#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ll_layouts.py — раскладка структур Minecraft из клиентских заголовков LeviLamina.

ЗАЧЕМ
  LeviLamina компилируется против SDK-заголовков, сгенерированных ИЗ PDB игры, и хранит членов
  классов в виде `::ll::TypedStorage<выравнивание, размер, тип> имя;` — то есть размер и тип
  каждого поля известны точно. Значит, из заголовков можно восстановить РАСКЛАДКУ классов игры
  (смещение и размер каждого поля) для той версии, под которую собран LeviLamina.

  Соответствие версий (из их docs/versions.md):
      LeviLamina 1.8.x -> клиент 1.21.124.2      LeviLamina 1.9.x -> клиент 1.21.132.1
      26.10.x -> 26.10.4   26.20.x -> 26.20.4   26.32.x -> 26.32.2   26.40.x -> 26.40.5   26.51.x -> 26.51.1

  Где взять заголовки:
      git clone --depth 1 --branch v1.9.9 --filter=blob:none --sparse https://github.com/LiteLDev/LeviLamina
      cd LeviLamina && git sparse-checkout set src-client

ИСПОЛЬЗОВАНИЕ
  python3 tools/ll_layouts.py /tmp/ll199 --list "LevelRenderer|ScreenContext|ClientInstance"
  python3 tools/ll_layouts.py /tmp/ll199 --class ClientInstance --show
  python3 tools/ll_layouts.py /tmp/ll199 --json layouts_1_21_132.json
  python3 tools/ll_layouts.py /tmp/ll199 --header sdk_client.hpp --filter "LevelRenderer|ScreenContext"

ВАЖНО
  * Смещения считаются по правилам MSVC x64: выравнивание каждого поля = указанное в TypedStorage,
    выравнивание класса = максимум из выравниваний полей и базовых классов.
  * Если базовый класс не найден среди разобранных (внешний), класс помечается `partial`,
    и его смещения считаются подозрительными — в отчёте это видно.
  * Поля без TypedStorage/UntypedStorage (обычные объявления) считаются по таблице размеров
    стандартных типов; неизвестный тип помечается `unknown_type` и ломает точность дальше по списку.
"""

from __future__ import annotations

import argparse
import json
import os
import pickle
import re
import sys
from collections import OrderedDict

# sizeof/alignof для типов, которые встречаются в обычных (не TypedStorage) объявлениях, MSVC x64
KNOWN_TYPES = {
    "bool": (1, 1), "char": (1, 1), "signed char": (1, 1), "unsigned char": (1, 1),
    "short": (2, 2), "unsigned short": (2, 2), "wchar_t": (2, 2), "char16_t": (2, 2),
    "int": (4, 4), "unsigned int": (4, 4), "long": (4, 4), "unsigned long": (4, 4),
    "float": (4, 4), "int32_t": (4, 4), "uint32_t": (4, 4), "unsigned": (4, 4),
    "long long": (8, 8), "unsigned long long": (8, 8), "double": (8, 8), "int64_t": (8, 8),
    "uint64_t": (8, 8), "size_t": (8, 8), "uintptr_t": (8, 8), "intptr_t": (8, 8),
    "std::string": (32, 8), "std::string_view": (16, 8), "std::u8string": (32, 8),
    "std::wstring": (32, 8), "glm::vec2": (8, 4), "glm::vec3": (12, 4), "glm::vec4": (16, 4),
    "glm::ivec3": (12, 4), "glm::mat4": (64, 4), "mce::Color": (16, 4), "Color": (16, 4),
    "BlockPos": (12, 4), "Vec3": (12, 4), "Vec2": (8, 4), "ChunkPos": (8, 4),
    "DimensionType": (4, 4), "ActorUniqueID": (8, 8), "ActorRuntimeID": (8, 8),
}


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def find_class_bodies(text: str):
    """Возвращает список (kind, name, bases, body) для class/struct с телом."""
    results = []
    pattern = re.compile(r"(?:^|\n)\s*(class|struct)\s+([A-Za-z_]\w*)\s*(?::\s*([^{;]*?))?\s*\{", re.S)
    for match in pattern.finditer(text):
        kind, name, bases = match.group(1), match.group(2), (match.group(3) or "")
        start = match.end()
        depth = 1
        pos = start
        while pos < len(text) and depth:
            char = text[pos]
            if char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
                if depth == 0:
                    break
            pos += 1
        results.append((kind, name, bases, text[start:pos], text[match.start():pos + 1]))
    return results


def parse_bases(bases: str):
    out = []
    for part in bases.split(","):
        part = part.strip()
        if not part:
            continue
        part = re.sub(r"^(public|private|protected|virtual)\s+", "", part)
        part = re.sub(r"^(public|private|protected|virtual)\s+", "", part)
        part = part.replace("::", " ").strip()
        if part:
            out.append(part.split()[-1])
    return out


STORAGE_RE = re.compile(
    r"::ll::(Typed|Untyped)Storage\s*<\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*(.+?))?\s*>\s*([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;")


def parse_members(body: str):
    """Возвращает список полей: dict(name, size, align, type, array, storage)."""
    members = []
    # отрезаем тела вложенных типов и тела функций, чтобы не путать их с полями
    cleaned = re.sub(r"\b(class|struct|enum|union)\s+\w+\s*(?::[^{;]*)?\{.*?\};", "", body, flags=re.S)
    cleaned = re.sub(r"\{.*?\}", "", cleaned, flags=re.S)

    consumed = []
    for match in STORAGE_RE.finditer(cleaned):
        kind, align, size, type_name, name, array = match.groups()
        align, size = int(align), int(size)
        count = 1
        if array:
            inner = array.strip("[]")
            digits = re.findall(r"\d+", inner)
            count = int(digits[0]) if digits else 1
        members.append({
            "name": name, "size": size * count, "align": align,
            "type": (type_name or "").strip(), "storage": kind,
            "count": count, "text": match.group(0).strip(),
        })
        consumed.append((match.start(), match.end()))

    # обычные объявления после отсечения хранилищ
    remainder = cleaned
    for start, end in sorted(consumed, reverse=True):
        remainder = remainder[:start] + " " * (end - start) + remainder[end:]

    SKIP_WORDS = ("MCAPI", "static", "friend", "using", "typedef", "template", "return", ";")
    for match in re.finditer(r"(?:^|[\n;])\s*((?:[A-Za-z_][\w:<>,\s\*&\[\]]*?))\s+([A-Za-z_]\w*)\s*(\[\s*\d*\s*\])?\s*;", remainder):
        type_text, name, array = match.group(1).strip(), match.group(2), match.group(3)
        if any(word in type_text for word in ("(", ")", "#", "=")):
            continue
        if any(type_text.split()[0].startswith(word) for word in SKIP_WORDS if type_text.split()):
            continue
        base_type = type_text.replace("::", " ").split()[-1] if not type_text.endswith("*") else type_text
        pointers = type_text.count("*")
        references = type_text.count("&")
        count = 1
        if array:
            digits = re.findall(r"\d+", array)
            count = int(digits[0]) if digits else 1
        if pointers or references:
            size, align = 8 * max(1, pointers), 8
            type_text = type_text.rstrip("&").strip() + (" *" * max(1, pointers))
        elif base_type in KNOWN_TYPES:
            size, align = KNOWN_TYPES[base_type]
            size *= count
            count = 1
        else:
            size, align = None, None
        members.append({
            "name": name, "size": size, "align": align, "type": type_text,
            "storage": "plain", "count": count, "unknown": size is None,
            "text": match.group(0).strip(),
        })
    return members


def compute_layout(name, parsed, resolved_cache, stack=None):
    """Считает offset/size для класса с учётом базовых классов (с защитой от циклов)."""
    if name in resolved_cache:
        return resolved_cache[name]
    stack = stack or set()
    if name in stack:
        return None  # циклическое наследование — не ошибка данных, просто не считаем
    parsed_entry = parsed.get(name)
    if parsed_entry is None:
        resolved_cache[name] = None
        return None
    stack = stack | {name}

    warnings = []
    offset = 0
    align = 1
    members_layout = []

    for base in parsed_entry["bases"]:
        if base == name:
            warnings.append("self_base")
            continue
        base_layout = compute_layout(base, parsed, resolved_cache, stack)
        if base_layout is None:
            warnings.append("unknown_base:%s" % base)
            continue
        base_size, base_align = base_layout["size"], base_layout["align"]
        offset = (offset + base_align - 1) // base_align * base_align
        offset += base_size
        align = max(align, base_align)

    for member in parsed_entry["members"]:
        if member.get("unknown") or not member.get("size"):
            warnings.append("unknown_type:%s" % member["name"])
            # размер неизвестен — дальше смещения недостоверны
            members_layout.append({**member, "offset": None})
            continue
        member_align = member["align"] or 1
        offset = (offset + member_align - 1) // member_align * member_align
        members_layout.append({**member, "offset": offset})
        offset += member["size"]
        align = max(align, member_align)

    size = (offset + align - 1) // align * align if offset else 0
    result = {
        "name": name,
        "kind": parsed_entry["kind"],
        "bases": parsed_entry["bases"],
        "members": members_layout,
        "size": size,
        "align": align,
        "partial": bool(warnings),
        "warnings": warnings,
    }
    resolved_cache[name] = result
    return result


def load_headers(root: str):
    parsed = OrderedDict()
    files = 0
    for current_dir, _dirs, names in os.walk(root):
        for name in names:
            if not name.endswith((".h", ".hpp")):
                continue
            path = os.path.join(current_dir, name)
            try:
                text = strip_comments(open(path, encoding="utf-8", errors="replace").read())
            except OSError:
                continue
            files += 1
            for kind, cls_name, bases, body, _full in find_class_bodies(text):
                if cls_name in parsed:
                    continue
                parsed[cls_name] = {
                    "kind": kind, "bases": parse_bases(bases),
                    "members": parse_members(body), "file": os.path.relpath(path, root),
                }
    return parsed, files


def main():
    parser = argparse.ArgumentParser(description="Раскладка структур MC из заголовков LeviLamina")
    parser.add_argument("root", help="корень src-client (или каталог с заголовками)")
    parser.add_argument("--class", dest="class_name", help="показать один класс")
    parser.add_argument("--list", help="regex по имени класса — вывести список с размерами")
    parser.add_argument("--show", action="store_true", help="печатать поля класса")
    parser.add_argument("--json", help="выгрузить все раскладки в JSON")
    parser.add_argument("--header", help="выгрузить C++ заголовок со структурами")
    parser.add_argument("--filter", help="regex для --json/--header")
    parser.add_argument("--limit", type=int, default=40, help="лимит строк в выводе")
    parser.add_argument("--cache", help="кеш разбора заголовков (pickle) — повторные запуски мгновенные")
    parser.add_argument("--stats", action="store_true", help="сколько классов разобрано полностью/частично")
    args = parser.parse_args()

    if args.cache and os.path.exists(args.cache) and os.path.getmtime(args.cache) > 0:
        with open(args.cache, "rb") as handle:
            parsed, files = pickle.load(handle)
        print("кеш: %s (файлов %d, классов %d)" % (args.cache, files, len(parsed)))
    else:
        parsed, files = load_headers(args.root)
        print("заголовков: %d, классов/структур с телом: %d" % (files, len(parsed)))
        if args.cache:
            with open(args.cache, "wb") as handle:
                pickle.dump((parsed, files), handle)
            print("кеш сохранён: %s" % args.cache)

    cache = {}
    def resolve(name):
        return compute_layout(name, parsed, cache)

    if args.stats:
        full = partial = empty = 0
        for name in parsed:
            layout = resolve(name)
            if not layout or not layout["size"]:
                empty += 1
            elif layout["partial"]:
                partial += 1
            else:
                full += 1
        print("\nклассов всего: %d" % len(parsed))
        print("  раскладка посчитана полностью: %d" % full)
        print("  частично (нет базы/неизвестный тип): %d" % partial)
        print("  не посчитано (пустые/без полей): %d" % empty)
        return 0

    if args.class_name:
        layout = resolve(args.class_name)
        if not layout:
            print("класс %s не найден" % args.class_name)
            return 1
        print("\n%s %s : %s  size=0x%X (%d), align=%d%s" % (
            layout["kind"], layout["name"], ", ".join(layout["bases"]) or "-",
            layout["size"], layout["size"], layout["align"],
            "  [PARTIAL: " + ", ".join(layout["warnings"][:4]) + "]" if layout["partial"] else ""))
        for member in layout["members"]:
            offset = "???" if member["offset"] is None else "0x%03X" % member["offset"]
            size = "?" if member.get("size") is None else str(member["size"])
            print("  +%-7s %-4s %-52s %s" % (offset, size, (member["type"] or "")[:52], member["name"]))
        return 0

    if args.list:
        pattern = re.compile(args.list)
        rows = []
        for name in parsed:
            if pattern.search(name):
                layout = resolve(name)
                if layout:
                    rows.append(layout)
        rows.sort(key=lambda item: item["size"], reverse=True)
        print("\n%-44s %8s %8s  %s" % ("класс", "size", "align", "базы/предупреждения"))
        for layout in rows[: args.limit]:
            note = ", ".join(layout["bases"][:2]) if layout["bases"] else ""
            if layout["partial"]:
                note += ("  ! " if note else "! ") + layout["warnings"][0]
            print("%-44s %8d %8d  %s" % (layout["name"], layout["size"], layout["align"], note[:70]))
        print("всего найдено: %d" % len(rows))
        return 0

    if args.json or args.header:
        pattern = re.compile(args.filter) if args.filter else None
        layouts = []
        for name in parsed:
            if pattern and not pattern.search(name):
                continue
            layout = resolve(name)
            if layout and layout["size"]:
                layouts.append(layout)
        layouts.sort(key=lambda item: (item["partial"], -item["size"]))
        if args.json:
            with open(args.json, "w", encoding="utf-8") as handle:
                json.dump({"classes": layouts, "count": len(layouts)}, handle, ensure_ascii=False, indent=1)
            print("JSON: %s (%d классов)" % (args.json, len(layouts)))
        if args.header:
            lines = ["#pragma once", "// auto-generated by tools/ll_layouts.py",
                     "#include <cstddef>", "#include <cstdint>", ""]
            CPP = {"bool": "bool", "char": "char", "int": "int", "float": "float", "double": "double",
                   "long": "long", "long long": "long long", "unsigned int": "unsigned int",
                   "unsigned char": "unsigned char", "short": "short", "unsigned short": "unsigned short",
                   "unsigned long long": "unsigned long long"}
            for layout in layouts:
                if layout["partial"]:
                    continue  # неполные раскладки в заголовок не пишем
                lines.append("// %s, size = 0x%X" % (layout["name"], layout["size"]))
                lines.append("struct %sLayout {" % layout["name"])
                cursor = 0
                pad = 0
                for member in layout["members"]:
                    if member["offset"] is None:
                        continue
                    if member["offset"] > cursor:
                        lines.append("    std::byte _pad%d[0x%X];" % (pad, member["offset"] - cursor))
                        pad += 1
                    cpp_type = CPP.get(member["type"], "std::byte")
                    if cpp_type == "std::byte":
                        lines.append("    /*+0x%03X*/ std::byte %s[0x%X]; // %s" % (
                            member["offset"], member["name"], member["size"], member["type"][:40]))
                    else:
                        lines.append("    /*+0x%03X*/ %s %s;" % (member["offset"], cpp_type, member["name"]))
                    cursor = member["offset"] + member["size"]
                if layout["size"] > cursor:
                    lines.append("    std::byte _padEnd[0x%X];" % (layout["size"] - cursor))
                lines.append("};")
                lines.append("static_assert(sizeof(%sLayout) == 0x%X);" % (layout["name"], layout["size"]))
                lines.append("")
            with open(args.header, "w", encoding="utf-8") as handle:
                handle.write("\n".join(lines))
            print("HEADER: %s" % args.header)
        return 0

    print("укажи --class / --list / --json / --header")
    return 0


if __name__ == "__main__":
    sys.exit(main())
