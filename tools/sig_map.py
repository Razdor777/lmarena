#!/usr/bin/env python3
"""Сопоставляет 124 захардкоженные сигнатуры проекта с именами символов 1.26.

Зачем: вместо поиска по байтам кода (ломается каждый апдейт) можно искать по имени
в symdb из bedrock-runtime-data. Для этого каждой записи DEFINE_SIG нужно сопоставить
имя символа в 1.26 — вот этим скрипт и занимается.

Вход:  src/SDK/SigManager.hpp  (DEFINE_SIG(Имя, "байты", тип, offset))
Выход: docs/migration-1.26/sig-to-symbol.md      — отчёт
       src/SDK/Generated/SigNames_1_26.hpp       — таблица для SymDB

Запуск:
    python3 tools/sig_map.py                       # с клонированием LeviLamina
    python3 tools/sig_map.py --ll /path/to/LeviLamina
    python3 tools/sig_map.py --emit-only           # без хидеров, только перевыпуск
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SIG_H = ROOT / "src" / "SDK" / "SigManager.hpp"
LL_DEFAULT = Path("/home/user/LeviLamina")
OUT_MD = ROOT / "docs" / "migration-1.26" / "sig-to-symbol.md"
OUT_HPP = ROOT / "src" / "SDK" / "Generated" / "SigNames_1_26.hpp"

# ручные правки: имя сигнатуры -> (статус, имя символа 1.26, комментарий)
SPECIAL = {
    "Keyboard_feed": ("GONE", "", "в 1.26 вход — событие ll::event::input::KeyInputEvent"),
    "MouseDevice_feed": ("GONE", "", "в 1.26 вход — событие ll::event::input::MouseInputEvent"),
    "ScreenView_setupAndRender": ("GONE", "", "в 1.26 рендер UI — событие RenderEvent"),
    "Mob_getCurrentSwingDuration": (
        "RENAMED", "Mob::getModifiedSwingDuration", "Item::getSwingDuration — это про предмет, не про моба"),
    "Mob_getJumpControlComponent": (
        "MOVED", "Mob::getJumpControlComponent", "компонент переехал в ECS: MobJumpControlComponent"),
    "BlockSource_fireBlockChanged": (
        "FOUND", "BlockSource::fireBlockChanged", "метод есть, виртуальный; транк $fireBlockChanged"),
    "glm_rotate": ("GONE", "", "статическая библиотека, символа в exe нет — считай сам"),
    "glm_rotateRef": ("GONE", "", "статическая библиотека, символа в exe нет — считай сам"),
    "glm_translateRef": ("GONE", "", "статическая библиотека, символа в exe нет — считай сам"),
    "glm_translateRef2": ("GONE", "", "статическая библиотека, символа в exe нет — считай сам"),
    "JSON_parse": ("GONE", "", "в 1.26 Json::Value парсится иначе"),
    "MainView_instance": ("GONE", "", "класса MainView в 1.26 нет"),
    "Actor_setPosition": (
        "RENAMED", "Actor::_setPos",
        "Actor.h:661 — в 1.26 setPosition переименован в _setPos (или teleportTo, Actor.h:380)"),
    "ResourcePackManager_composeFullStackBp": (
        "RENAMED", "ResourcePackManager::composeFullStack",
        "ResourcePackManager.h:142 — суффикс Bp в 1.26 отпал"),
    "ClientInstance_mBgfx": (
        "GONE", "", "bgfx в 1.26 внешняя библиотека (src-client/mc/external/bgfx), поля в ClientInstance нет"),
    "ContainerScreenController_tryBeginTransition": (
        "GONE", "", "метода нет ни в ContainerScreenController, ни в базовых классах"),
    "RakNet_RakPeer_sendImmediate": (
        "GONE", "", "RakNet вынесен во внешний модуль; в 1.26 основной транспорт — WebRTC"),
    "ComplexInventoryTransaction_vtable": (
        "FOUND", "ComplexInventoryTransaction::ComplexInventoryTransaction",
        "vtаблица класса; адрес бери от конструктора"),
    "ConnectionRequest_create": ("GONE", "", "сетевой стек переехал (WebRTC)"),
}

# классы, которых в 1.26 нет
DEAD_CLASSES = {"BlockLegacy", "ConcreteBlockLegacy", "MinecraftSim", "MainView", "BedrockPlatformUWP",
                "UIProfanityContext", "CameraDirectLookSystemUtil"}


@dataclass
class Sig:
    name: str
    pattern: str
    sigtype: str
    offset: int
    cls: str = ""
    member: str = ""
    status: str = "?"
    symbol: str = ""
    note: str = ""


SIG_RE = re.compile(
    r'DEFINE_SIG\(\s*(\w+)\s*,\s*((?:\s*"[^"]*"\s*)+),\s*SigType::(\w+)\s*,\s*(-?\d+)\s*\)',
    re.M,
)


def parse_sigs(path: Path) -> list[Sig]:
    text = path.read_text(encoding="utf8", errors="ignore")
    out: list[Sig] = []
    for m in SIG_RE.finditer(text):
        pattern = " ".join(re.findall(r'"([^"]*)"', m.group(2)))
        s = Sig(name=m.group(1), pattern=pattern, sigtype=m.group(3), offset=int(m.group(4)))
        if "_" in s.name:
            s.cls, s.member = s.name.split("_", 1)
        else:
            s.cls, s.member = s.name, ""
        out.append(s)
    return out


def ensure_ll(ll: Path) -> bool:
    if (ll / "src" / "mc").is_dir():
        return True
    print(f"[i] клонирую LeviLamina в {ll} ...", file=sys.stderr)
    subprocess.run(["git", "clone", "--depth", "1", "-q",
                    "https://github.com/LiteLDev/LeviLamina.git", str(ll)], check=False)
    return (ll / "src" / "mc").is_dir()


def build_index(ll: Path) -> dict[str, list[Path]]:
    idx: dict[str, list[Path]] = {}
    for base in (ll / "src", ll / "src-client", ll / "src-server"):
        if not base.is_dir():
            continue
        for p in base.rglob("*.h"):
            idx.setdefault(p.stem, []).append(p)
    return idx


def pick_class_file(idx: dict[str, list[Path]], cls: str) -> Path | None:
    cands = idx.get(cls, [])
    if not cands:
        return None

    def richness(p: Path) -> int:
        try:
            t = p.read_text(encoding="utf8", errors="ignore")
        except OSError:
            return 0
        return t.count("\n") + 20 * t.count("ll::TypedStorage<")

    return max(cands, key=richness)


MEMBER_RE = re.compile(r"\b(\w+)\s*\(")


def _find_call(text: str, name: str) -> int | None:
    """Номер строки, где встречается вызов `name(` (с учётом транка $)."""
    for m in re.finditer(rf"\b\$?{re.escape(name)}\s*\(", text):
        return text.count("\n", 0, m.start()) + 1
    # без учёта регистра (SetPosition vs setPosition)
    for m in re.finditer(rf"\b\$?{re.escape(name)}\s*\(", text, re.I):
        return text.count("\n", 0, m.start()) + 1
    return None


def _find_field(text: str, name: str) -> int | None:
    for m in re.finditer(rf"\b{re.escape(name)}\s*[;\[]", text):
        return text.count("\n", 0, m.start()) + 1
    return None


def find_in_class(hfile: Path, member: str) -> tuple[str, int] | None:
    """Ищет метод или поле в хидере класса. Возвращает (имя, строка)."""
    text = hfile.read_text(encoding="utf8", errors="ignore")
    base = re.sub(r"_(jnz|test|bytepatch|vtable|inlined2?|Ref2?|ctor)$", "", member)
    short = re.sub(r"^(get|set|is|has)", "", base) if base[:1].isupper() or base[:3] in (
        "get", "set", "is_") else base
    for cand in filter(None, {member, base, f"_{base}", short, f"_{short}"}):
        line = _find_call(text, cand)
        if line:
            return cand, line
    # поле класса (mBgfx, mFov и т.п.)
    for cand in filter(None, {member, base}):
        line = _find_field(text, cand)
        if line:
            return cand, line
    return None


def find_method(hfile: Path, member: str) -> tuple[str, int] | None:
    """Ищет метод в хидере. Возвращает (найденное имя, строка)."""
    text = hfile.read_text(encoding="utf8", errors="ignore")
    # сначала точное имя (в т.ч. с $ — транк)
    for m in re.finditer(rf"\b\$?{re.escape(member)}\s*\(", text):
        line = text.count("\n", 0, m.start()) + 1
        return member, line
    # потом без суффиксов _jnz/_test/_bytepatch/_vtable, которые в имени сигнатуры
    base = re.sub(r"_(jnz|test|bytepatch|vtable|inlined2?|Ref2?|ctor)$", "", member)
    if base and base != member:
        for m in re.finditer(rf"\b\$?{re.escape(base)}\s*\(", text):
            line = text.count("\n", 0, m.start()) + 1
            return base, line
    return None


def global_search(ll: Path, member: str) -> tuple[str, str, str] | None:
    """Ищет имя по всем хидерам. Возвращает (путь:строка, класс, что нашли)."""
    base = re.sub(r"_(jnz|test|bytepatch|vtable|inlined2?|Ref2?|ctor)$", "", member)
    pats = [
        (rf"\b\$?{re.escape(base)}\s*\(", "метод"),
        (rf"\b{re.escape(base)}\s*[;\[]", "поле"),
    ]
    cands = {member, base, f"_{base}"}
    for d in (ll / "src", ll / "src-client", ll / "src-server"):
        if not d.is_dir():
            continue
        for hf in d.rglob("*.h"):
            try:
                text = hf.read_text(encoding="utf8", errors="ignore")
            except OSError:
                continue
            for cand in cands:
                for pat, kind in pats:
                    p = pat.replace(re.escape(base), re.escape(cand))
                    m = re.search(p, text)
                    if m:
                        line = text.count("\n", 0, m.start()) + 1
                        owner = ""
                        for om in re.finditer(r"(?:class|struct)\s+(\w+)", text[: m.start()]):
                            owner = om.group(1)
                        return f"{hf.relative_to(ll)}:{line}", owner, kind
    return None


def resolve(sigs: list[Sig], ll: Path) -> None:
    if not ll:
        return
    idx = build_index(ll)
    for s in sigs:
        if s.name in SPECIAL:
            s.status, s.symbol, s.note = SPECIAL[s.name]
            continue
        if s.cls in DEAD_CLASSES:
            s.status, s.symbol, s.note = "GONE", "", f"класса {s.cls} в 1.26 нет"
            continue
        hf = pick_class_file(idx, s.cls)
        if hf is None:
            s.status, s.note = "NO_CLASS", f"хидер {s.cls}.h не найден"
            continue
        hit = find_in_class(hf, s.member)
        if hit:
            name, line = hit
            rel = hf.relative_to(ll)
            s.status = "FOUND"
            s.symbol = f"{s.cls}::{name}"
            s.note = f"{rel}:{line}"
            continue
        # в своём классе нет — ищем, куда переехало
        g = global_search(ll, s.member)
        if g:
            where, owner, kind = g
            s.status = "MOVED"
            s.symbol = f"{owner}::{s.member}" if owner else s.member
            s.note = f"переехало: {where} ({kind})"
        else:
            s.status = "NOT_IN_CLASS"
            s.symbol = ""
            s.note = f"{s.cls} есть, метода {s.member} в нём нет"


def render_md(sigs: list[Sig], ll: Path | None) -> str:
    from collections import Counter
    c = Counter(s.status for s in sigs)
    L = [
        "# Сигнатуры проекта → символы 1.26",
        "",
        "Таблица нужна, чтобы заменить поиск по байтам (`DEFINE_SIG`) на поиск по имени в symdb.",
        "",
        f"Источник: `src/SDK/SigManager.hpp` ({len(sigs)} записей).",
        "",
        "Статусы: " + ", ".join(f"**{k}** {v}" for k, v in sorted(c.items(), key=lambda x: -x[1])),
        "",
        "| Сигнатура | Класс | Член | Символ 1.26 | Статус | Где |",
        "|---|---|---|---|---|---|",
    ]
    for s in sigs:
        sym = f"`{s.symbol}`" if s.symbol else "—"
        L.append(f"| `{s.name}` | `{s.cls}` | `{s.member}` | {sym} | {s.status} | {s.note} |")
    L += [
        "",
        "## Как этим пользоваться",
        "",
        "1. Скачать `bedrock-runtime-data` версии `26.51.1-client.6` "
        "(релиз на github.com/LiteLDev/bedrock-runtime-data).",
        "2. Распаковать в `tools/data/bedrock_runtime_data/`.",
        "3. `SymDB::instance().load(...)` прочитает файлы, а `SigManager` при неудачном",
        "   поиске по байтам попробует найти адрес по имени из этой таблицы.",
        "",
        "Там, где статус GONE/NO_CLASS, имени нет — такие места надо переписывать",
        "на события или искать вручную.",
    ]
    return "\n".join(L) + "\n"


def render_hpp(sigs: list[Sig]) -> str:
    L = [
        "// СГЕНЕРЕНО tools/sig_map.py — не править руками",
        "//",
        "// Сопоставление имён из SigManager с именами символов 1.26.",
        "// SymDB ищет по `symbol`; если пусто — имя в 1.26 неизвестно.",
        "",
        "#pragma once",
        "",
        "#include <cstddef>",
        "#include <string>",
        "",
        "namespace SigNames {",
        "",
        "struct Entry {",
        "    const char* sig;      // имя из SigManager",
        "    const char* symbol;   // имя символа в 1.26, \"\" если неизвестно",
        "    const char* status;   // FOUND / RENAMED / MOVED / GONE / NO_CLASS / NOT_IN_CLASS",
        "};",
        "",
        "inline constexpr Entry kTable[] = {",
    ]
    for s in sigs:
        L.append(f'    {{ "{s.name}", "{s.symbol}", "{s.status}" }},')
    L += [
        "};",
        "",
        "inline constexpr size_t kCount = sizeof(kTable) / sizeof(kTable[0]);",
        "",
        "// Имя символа 1.26 для записи SigManager; пустая строка, если имя неизвестно.",
        "inline const char* symbolOf(const std::string& sigName) {",
        "    for (size_t i = 0; i < kCount; ++i)",
        "        if (sigName == kTable[i].sig) return kTable[i].symbol;",
        '    return "";',
        "}",
        "",
        "}  // namespace SigNames",
        "",
    ]
    return "\n".join(L)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--ll", type=Path, default=LL_DEFAULT, help="путь к клону LeviLamina")
    ap.add_argument("--emit-only", action="store_true", help="не читать хидеры, только перевыпустить файлы")
    a = ap.parse_args()

    sigs = parse_sigs(SIG_H)
    if not sigs:
        print(f"[!] не нашёл DEFINE_SIG в {SIG_H}", file=sys.stderr)
        return 1
    print(f"[i] сигнатур: {len(sigs)}")

    if not a.emit_only:
        if ensure_ll(a.ll):
            resolve(sigs, a.ll)
        else:
            print("[!] LeviLamina недоступна — статусы не посчитаны", file=sys.stderr)

    OUT_MD.parent.mkdir(parents=True, exist_ok=True)
    OUT_MD.write_text(render_md(sigs, a.ll if not a.emit_only else None), encoding="utf8")
    OUT_HPP.parent.mkdir(parents=True, exist_ok=True)
    OUT_HPP.write_text(render_hpp(sigs), encoding="utf8")

    from collections import Counter
    c = Counter(s.status for s in sigs)
    print("[i] статусы:", dict(sorted(c.items(), key=lambda x: -x[1])))
    print(f"[+] {OUT_MD.relative_to(ROOT)}")
    print(f"[+] {OUT_HPP.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
