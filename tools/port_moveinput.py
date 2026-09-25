#!/usr/bin/env python3
"""
port_moveinput.py — механическая правка вызывающего кода под новый
MoveInputComponent (1.26): поля-флаги стали битами, поэтому обращение к ним
заменяется на методы.

Правила (только для обращений через -> и ., «голые» имена не трогаем):

    obj->mIsSneakDown = v;   ->  obj->setSneakDown(v);
    obj->mIsSneakDown        ->  obj->isSneakDown()
    obj->mIsJumping = v;     ->  obj->setJumping(v);
    obj->mIsJumping          ->  obj->isJumping()
    obj->mIsJumping2 = v;    ->  obj->setJumpingCurrentlyDown(v);
    obj->mIsJumping2         ->  obj->isJumpingCurrentlyDown()
    obj->mIsSprinting = v;   ->  obj->setSprinting(v);
    obj->setmIsSprinting(v)  ->  obj->setSprinting(v);
    obj->mIsSprinting        ->  obj->isSprinting()
    obj->mForward/backward/left/right  ->  obj->isForward()/...  и setXxx(v)
    obj->mMoveVector = v;    ->  obj->setMoveVector(v);
    obj->mMoveVector         ->  obj->moveVector()
    obj->mIsMoveLocked = v;  ->  obj->setMoveLocked(v);
    obj->mIsMoveLocked       ->  obj->isMoveLocked()

Запуск:
    python3 tools/port_moveinput.py --dry-run   # показать, что изменится
    python3 tools/port_moveinput.py             # применить

Правка обратима: git checkout <файлы>.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

# имя поля -> (геттер, сеттер)
MAP = {
    "mIsSneakDown": ("isSneakDown", "setSneakDown"),
    "mIsJumping": ("isJumping", "setJumping"),
    "mIsJumping2": ("isJumpingCurrentlyDown", "setJumpingCurrentlyDown"),
    "mIsSprinting": ("isSprinting", "setSprinting"),
    "mForward": ("isForward", "setForward"),
    "mBackward": ("isBackward", "setBackward"),
    "mLeft": ("isLeft", "setLeft"),
    "mRight": ("isRight", "setRight"),
    "mIsMoveLocked": ("isMoveLocked", "setMoveLocked"),
    "mMoveVector": ("moveVector", "setMoveVector"),
}

SKIP_FILES = {
    "src/SDK/Minecraft/Actor/Components/MoveInputComponent.hpp",
}


def port_text(text: str) -> tuple[str, int]:
    n = 0

    # 1. старый метод setmIsSprinting(v) -> setSprinting(v)
    text, k = re.subn(r"->setmIsSprinting\(", "->setSprinting(", text)
    n += k

    # 2. присваивания: obj->field = expr;   /   obj.field = expr;
    for field, (_get, setter) in MAP.items():
        pat = re.compile(r"(->|\.)\s*" + field + r"\s*=\s*([^;]+);")
        def rep(m: re.Match) -> str:
            return f"{m.group(1)}{setter}({m.group(2)});"
        text, k = pat.subn(rep, text)
        n += k

    # 3. чтения: obj->field  (не перед '=')
    for field, (getter, _set) in MAP.items():
        pat = re.compile(r"(->|\.)" + field + r"\b(?!\s*=[^=])")
        text, k = pat.subn(lambda m: f"{m.group(1)}{getter}()", text)
        n += k

    return text, n


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    total = 0
    changed: list[str] = []
    for p in sorted(REPO.rglob("*")):
        if not p.is_file() or p.suffix.lower() not in (".cpp", ".hpp", ".h"):
            continue
        rel = p.relative_to(REPO).as_posix()
        if rel in SKIP_FILES or rel.startswith("docs/") or rel.startswith("tools/"):
            continue
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        new, n = port_text(text)
        if n and new != text:
            total += n
            changed.append(f"  {rel}: {n}")
            if not args.dry_run:
                p.write_text(new, encoding="utf-8")

    print(f"правок: {total}")
    print("\n".join(changed) if changed else "  (нечего менять)")
    if args.dry_run:
        print("\nэто dry-run, файлы не изменены")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
