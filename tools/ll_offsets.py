#!/usr/bin/env python3
"""
ll_offsets.py — считает смещения полей класса по заголовкам LeviLamina.

Заголовки LeviLamina описывают поля как
    ::ll::TypedStorage<Align, Size, Type> mName;
в порядке объявления. Значит смещение можно посчитать: идём по порядку,
выравниваем по Align, прибавляем Size.

Важно:
  - смещения считаются от начала того блока полей, который описан в хидере;
    если у класса есть базовый класс, его размер в хидере неизвестен —
    поэтому для таких классов выдаётся пометка BASE_UNKNOWN;
  - числа РАСЧЁТНЫЕ. Это не замена IDA, а способ не переснимать всё руками.

Запуск:
    python3 tools/ll_offsets.py MoveInputComponent StateVectorComponent
    python3 tools/ll_offsets.py --json MoveInputComponent
    python3 tools/ll_offsets.py --all-components      # все компоненты из src/SDK проекта
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DEFAULT_LL = Path("/home/user/LeviLamina")

FIELD_RE = re.compile(
    r"::ll::TypedStorage<\s*(\d+)\s*,\s*(\d+)\s*,\s*(.+?)>\s*(\w+)\s*(\[\s*\d+\s*\])?\s*;"
)
BASE_RE = re.compile(r"^\s*(?:class|struct)\s+(\w+)\s*:\s*public\s+(.+?)\s*\{", re.M)
DECL_RE = re.compile(r"^\s*(?:class|struct)\s+" + r"(\w+)" + r"\b[^{;]*\{", re.M)


def find_class_file(ll: Path, name: str) -> Path | None:
    """Выбираем файл, где класса больше всего описан:
    у одного имени бывает несколько хидеров (настоящий + клиентская заглушка),
    и заглушка с 2 полями нам не нужна."""
    best, best_score = None, -1
    for base in ("src-client", "src", "src-server"):
        d = ll / base / "mc"
        if not d.exists():
            continue
        for h in d.rglob(name + ".h"):
            text = h.read_text(encoding="utf-8", errors="replace")
            if not re.search(r"^\s*(?:class|struct)\s+" + re.escape(name) + r"\b\s*(:|\{)", text, re.M):
                continue
            score = len(FIELD_RE.findall(text))
            if "/src-client/" in h.as_posix():
                score += 1  # при равенстве предпочитаем клиентский (проект клиентский)
            if score > best_score:
                best, best_score = h, score
    return best


def class_body(text: str, name: str) -> str | None:
    m = re.search(r"^\s*(?:class|struct)\s+" + re.escape(name) + r"\b.*?\n(.*?)^};", text, re.S | re.M)
    return m.group(1) if m else None


def compute(ll: Path, name: str, depth: int = 0) -> dict | None:
    if depth > 3:
        return None
    hfile = find_class_file(ll, name)
    if hfile is None:
        return None
    text = hfile.read_text(encoding="utf-8", errors="replace")
    body = class_body(text, name)
    if body is None:
        return None

    base_m = BASE_RE.search(text)
    bases = [b.strip().lstrip(":").replace("::", "::") for b in (base_m.group(2).split(",") if base_m else [])]
    bases = [b.split("::")[-1] for b in bases]

    fields: list[dict] = []
    off = 0
    for m in FIELD_RE.finditer(body):
        align, size, typ, fname, arr = m.group(1), m.group(2), m.group(3).strip(), m.group(4), m.group(5)
        a, s = int(align), int(size)
        if a > 1 and off % a:
            off += a - (off % a)
        fields.append({"name": fname, "offset": off, "size": s, "align": a,
                       "type": typ + (arr if arr else "")})
        off += s

    return {
        "class": name,
        "file": str(hfile.relative_to(ll)),
        "bases": bases,
        "fields": fields,
        "size": off,
        "base_unknown": bool(bases),
    }


def render(info: dict) -> str:
    if info is None:
        return "  (не найдено в хидерах LeviLamina)"
    lines = [f"  {info['class']}  [{info['file']}]  расчётный размер: 0x{info['size']:X}"
             + ("  ⚠ есть базовый класс — смещения от начала своего блока" if info["base_unknown"] else "")]
    for f in info["fields"]:
        lines.append(f"    0x{f['offset']:03X}  {f['name']:38} {f['type']}  ({f['size']} байт)")
    if not info["fields"]:
        lines.append("    (поля не описаны — layout неизвестен)")
    return "\n".join(lines)


def project_components(root: Path) -> list[str]:
    out = []
    d = root / "src" / "SDK" / "Minecraft"
    for p in sorted(d.rglob("*.hpp")):
        t = p.read_text(encoding="utf-8", errors="replace")
        for m in re.finditer(r"(?:class|struct)\s+(\w+)\s*[^{;]*\{", t):
            out.append(m.group(1))
    return sorted(set(out))


def emit_header(root: Path, ll: Path, infos: dict[str, dict | None], path: Path) -> int:
    """Пишет src/SDK/Generated/Offsets_1_26.hpp с расчётными смещениями."""
    lines = [
        "//",
        "// СГЕНЕРИРОВАНО tools/ll_offsets.py — НЕ ПРАВИТЬ РУКАМИ.",
        "// Источник: заголовки LeviLamina 26.51 (TypedStorage<Align, Size, Type>).",
        "//",
        "// Смещения РАСЧЁТНЫЕ: поля идут в порядке объявления, выравнивание учитывается.",
        "// Классы с базовым классом помечены BASE_UNKNOWN — смещение от начала своего блока.",
        "// Перед использованием в бою проверяй в IDA (это экономия времени, не замена реверсу).",
        "//",
        "#pragma once",
        "",
        "#include <cstddef>",
        "",
        "namespace Offsets_1_26 {",
        "",
    ]
    n = 0
    for name, info in sorted(infos.items()):
        if not info or not info["fields"]:
            continue
        n += 1
        lines.append(f"// {info['class']}  [{info['file']}]  расчётный размер 0x{info['size']:X}"
                     + ("  ⚠ BASE_UNKNOWN" if info["base_unknown"] else ""))
        lines.append(f"namespace {name} {{")
        for f in info["fields"]:
            lines.append(f"    constexpr ptrdiff_t {f['name']} = 0x{f['offset']:X};  // {f['type']}, {f['size']} байт")
        lines.append(f"    constexpr ptrdiff_t Size = 0x{info['size']:X};")
        lines.append("}  // namespace " + name)
        lines.append("")
    lines.append("}  // namespace Offsets_1_26")
    lines.append("")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8")
    return n


def emit_md(root: Path, ll: Path, infos: dict[str, dict | None], path: Path) -> tuple[int, int]:
    """Пишет docs/migration-1.26/audit-sdk.md — аудит всех классов SDK."""
    found = [i for i in infos.values() if i and i["fields"]]
    empty = [i for i in infos.values() if i and not i["fields"]]
    missing = [n for n, i in infos.items() if i is None]

    usages: dict[str, int] = {}
    for p in (root / "src").rglob("*"):
        if p.suffix.lower() not in (".cpp", ".hpp", ".h"):
            continue
        t = p.read_text(encoding="utf-8", errors="replace")
        for n in infos:
            c = len(re.findall(r"\b" + re.escape(n) + r"\b", t))
            if c:
                usages[n] = usages.get(n, 0) + c

    lines = [
        "# Аудит SDK: классы проекта против заголовков LeviLamina 26.51",
        "",
        f"- Проверено классов из `src/SDK/Minecraft/**`: **{len(infos)}**",
        f"- С описанным layout'ом (можно взять смещения): **{len(found)}**",
        f"- Класс есть, но поля не описаны: **{len(empty)}**",
        f"- Класс в 1.26 не найден: **{len(missing)}**",
        "",
        "Смещения посчитаны `tools/ll_offsets.py` по `TypedStorage<Align, Size, Type>`",
        "и лежат в `src/SDK/Generated/Offsets_1_26.hpp`.",
        "",
        "## Классы с известным layout'ом 1.26",
        "",
        "| Класс | Хидер LeviLamina | Размер | Полей | Упоминаний в проекте |",
        "|---|---|---|---|---|",
    ]
    for i in sorted(found, key=lambda x: -usages.get(x["class"], 0)):
        lines.append(f"| `{i['class']}` | `{i['file']}` | 0x{i['size']:X} | {len(i['fields'])} "
                     f"| {usages.get(i['class'], 0)} |")
    lines += ["", "## Класс есть, но поля не описаны (layout неизвестен)", "",
              "| Класс | Хидер | Упоминаний |", "|---|---|---|"]
    for i in sorted(empty, key=lambda x: -usages.get(x["class"], 0)):
        lines.append(f"| `{i['class']}` | `{i['file']}` | {usages.get(i['class'], 0)} |")
    lines += ["", "## Класс не найден в 1.26 (переименован/удалён/кастомный)", "",
              "| Класс | Упоминаний |", "|---|---|"]
    for n in sorted(missing, key=lambda x: -usages.get(x, 0)):
        lines.append(f"| `{n}` | {usages.get(n, 0)} |")
    lines.append("")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8")
    return len(found), len(missing)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("classes", nargs="*")
    ap.add_argument("--ll", default=str(DEFAULT_LL))
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--all-components", action="store_true")
    ap.add_argument("--emit-header", metavar="PATH", help="сгенерировать C++-заголовок со смещениями")
    ap.add_argument("--emit-md", metavar="PATH", help="сгенерировать markdown-аудит SDK")
    args = ap.parse_args()

    ll = Path(args.ll)
    root = REPO

    names = list(args.classes)
    if args.all_components:
        names += project_components(root)
    if not names:
        print("укажите класс(ы) или --all-components")
        return 1

    result = {}
    for n in names:
        info = compute(ll, n)
        result[n] = info
        if not args.json and not args.emit_header and not args.emit_md:
            print(render(info) if info else f"  {n}: (не найдено)")

    if args.emit_header:
        cnt = emit_header(root, ll, result, root / args.emit_header)
        print(f"сгенерирован {args.emit_header} (классов: {cnt})")
    if args.emit_md:
        f, m = emit_md(root, ll, result, root / args.emit_md)
        print(f"сгенерирован {args.emit_md} (с layout: {f}, не найдено: {m})")
    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
