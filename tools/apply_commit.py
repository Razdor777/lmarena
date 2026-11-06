#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Применяет исправления 26.62 двумя возможными способами — выбирай любой.

    python apply_commit.py --check                     # только показать, что будет
    python apply_commit.py --mode git                  # взять файлы из коммита (git)
    python apply_commit.py --mode patches              # применить правки текстом (без git)
    python apply_commit.py --mode patches --root C:\\path\\to\\repo

Режимы.

  git      Если папка — git-репозиторий и в ней есть нужные коммиты, файлы
           просто достаются из них (`git show <коммит>:<путь>`) и кладутся на
           место — как есть, без слияния. Ничего не пушится, ветка не меняется.
           Cherry-pick намеренно НЕ используется: на другой истории он тянет
           чужие переименования (DEXKO_ → SOLSTICE_) и портит файлы.

  patches  Git не нужен вообще. Те же 11 файлов правятся по тексту: удаляется
           вторая секция «Debug dump», квалифицируется ItemStackBase::,
           убираются недостижимые catch, C5056/STRING(), добавляются
           SOLSTICE_* в генератор и #pragma once в шесть заголовков.
           Поиск идёт по маркерам, а не по номерам строк, поэтому работает
           на любой копии — со сдвинутыми строками и другими отступами.

Если ничего не указано, скрипт сам решает: git есть и коммит найден →
режим git, иначе → patches.

Перед изменением файлы копируются в backup_apply_commit_<время>/ (отключается
флагом --no-backup). Скрипт идемпотентный: уже применённое пропускает.
"""

import argparse
import os
import shutil
import subprocess
import sys
import time

# Коммиты, из которых берутся готовые файлы (режим git).
COMMITS = ["259916a", "310eab6", "0667723", "2d77f4f"]

# Те же 11 файлов, которые правятся текстом (режим patches).
FILES = [
    r"26.62/GenerateBuildInfo.cmake",
    r"26.62/src/Features/Auth/Authorization.hpp",
    r"26.62/src/Features/Auth/CustomCryptor.hpp",
    r"26.62/src/Features/Command/Commands/BuildInfoCommand.cpp",
    r"26.62/src/Features/Command/Commands/SnipeCommand.hpp",
    r"26.62/src/Features/Command/Commands/VclipCommand.hpp",
    r"26.62/src/Features/Events/WindowResizeEvent.hpp",
    r"26.62/src/Features/Modules/ModuleManager.cpp",
    r"26.62/src/Features/Modules/Player/ContainerActions.cpp",
    r"26.62/src/Features/Modules/Player/OreMinerV2.hpp",
    r"26.62/src/SDK/Minecraft/Network/Packets/CameraInstructionPacket.hpp",
]

# Файлы инструментов из коммита 259916a — только в режиме git.
TOOLS = [
    r"tools/check_duplicates.py",
    r"tools/check_unity_collisions.py",
    r"tools/check_header_guards.py",
]

# Эти файлы в git-режиме пропускаем: check_all.sh в чужой копии может быть
# другой, а docs править из коммита бессмысленно.
GIT_SKIP = (r"tools/check_all.sh", r"docs/")


# ============================================================================
# Режим git
# ============================================================================

def git(root, *args, **kw):
    return subprocess.run(["git"] + list(args), cwd=root, capture_output=True, text=True, **kw)


def is_git_repo(root):
    if not shutil.which("git"):
        return False
    r = git(root, "rev-parse", "--is-inside-work-tree")
    return r.returncode == 0 and r.stdout.strip() == "true"


def have_commit(root, commit):
    return git(root, "cat-file", "-e", commit + "^{commit}").returncode == 0


def run_git_mode(root, check, backup_dir=None):
    have_any = [c for c in COMMITS if have_commit(root, c)]
    if not have_any:
        print("  коммиты не найдены локально — можно попробовать `git fetch origin`, "
              "но проще запустить --mode patches")
        return None

    written = skipped = problems = 0
    for commit in have_any:
        listing = git(root, "show", "--name-only", "--format=", commit)
        paths = [p.strip() for p in listing.stdout.splitlines() if p.strip()]
        if not paths:
            continue
        print("  коммит %s — файлов: %d" % (commit, len(paths)))
        for rel in paths:
            if rel.startswith(GIT_SKIP):
                continue
            target = os.path.join(root, rel)
            blob = git(root, "show", "%s:%s" % (commit, rel))
            if blob.returncode != 0:
                continue                     # файл в этом коммите удалён
            new = blob.stdout
            old = ""
            if os.path.isfile(target):
                with open(target, "r", encoding="utf-8", errors="ignore", newline="") as f:
                    old = f.read()
            if old == new:
                skipped += 1
                continue
            print("     + %-64s из %s" % (rel, commit))
            written += 1
            if not check:
                if backup_dir and os.path.isfile(target):
                    dst = os.path.join(backup_dir, rel)
                    os.makedirs(os.path.dirname(dst), exist_ok=True)
                    shutil.copy2(target, dst)
                os.makedirs(os.path.dirname(target) or ".", exist_ok=True)
                with open(target, "w", encoding="utf-8", newline="") as f:
                    f.write(new)
    print("\n  изменено файлов: %d, уже было: %d, проблем: %d%s"
          % (written, skipped, problems, " (--check: ничего не записано)" if check else ""))
    return problems == 0


# ============================================================================
# Режим patches — те же правки текстом
# ============================================================================

OREMINER_HEADER_MARK = "// Debug dump"
OREMINER_TAIL_MARK = "static const char *oreTypeName(int blockId);"
OREMINER_STALE_MARKS = ("deepslate", "mDebugCsvPath")
OREMINER_ANCHOR = "// Строка «сырого» дампа (Dump Seen): блок, который модуль увидел."
OREMINER_WARNING = [
    "ВАЖНО: структура и всё ниже — ЕДИНСТВЕННОЕ объявление дампа в классе.",
    "При слиянии корневой версии с 26.62-версией секция «Debug dump» попала в",
    "файл дважды (два struct DebugRow, два debugStart/debugFlush/debugStop/",
    "oreTypeName) — MSVC падал с C2011/C2535/C2086. Вторая копия удалена.",
]


def _is_sep(line):
    t = line.strip()
    if not t.startswith("//"):
        return False
    body = t.lstrip("/").strip()
    return len(body) >= 8 and set(body) <= {"=", "-"}


def _sections(lines):
    out = []
    for i, ln in enumerate(lines):
        if ln.strip() != OREMINER_HEADER_MARK:
            continue
        start = i - 1 if i > 0 and _is_sep(lines[i - 1]) else i
        end = None
        for j in range(i, len(lines)):
            if OREMINER_TAIL_MARK in lines[j]:
                end = j
                break
        out.append((start, end))
    return out


def fix_oreminer(text):
    lines = text.split("\n")
    sections = _sections(lines)
    if not sections:
        return text, "НЕ НАЙДЕНО: секция «Debug dump» — пропущено"
    if len(sections) == 1:
        return text, "уже исправлено (секция одна)"

    stale = []
    for start, end in sections:
        body = "\n".join(lines[start:(end + 1 if end is not None else start + 1)])
        if any(m in body for m in OREMINER_STALE_MARKS):
            stale.append((start, end))

    if len(stale) != len(sections) - 1:
        return text, "НЕ НАЙДЕНО: неясно, какая секция устаревшая — пропущено"
    if any(end is None for _, end in stale):
        return text, "НЕ НАЙДЕНО: у секции нет конца — пропущено"

    removed = 0
    for start, end in sorted(stale, reverse=True):
        del lines[start:end + 1]
        removed += end - start + 1

    if "ВТОРАЯ копия удалена" not in text:
        for i, ln in enumerate(lines):
            if OREMINER_ANCHOR in ln:
                indent = ln[:len(ln) - len(ln.lstrip())] or "  "
                lines[i + 1:i + 1] = [indent + "// " + w for w in OREMINER_WARNING]
                break

    return "\n".join(lines), "удалено строк: %d (секций: %d)" % (removed, len(stale))


def fix_container(text):
    lines = text.split("\n")
    old_stmt = "armor->mValid_DeprecatedSeeComment=false;"
    new_stmt = "armor->ItemStackBase::mValid_DeprecatedSeeComment=false;"
    note = []
    old_idx = [i for i, ln in enumerate(lines) if old_stmt in ln and new_stmt not in ln]
    new_idx = [i for i, ln in enumerate(lines) if new_stmt in ln]
    if not old_idx and not new_idx:
        return text, "НЕ НАЙДЕНО: строка сброса брони — пропущено"

    for i in reversed(old_idx):
        if new_idx:
            del lines[i]
            note.append("удалена устаревшая строка")
        else:
            line = lines[i].replace(old_stmt, new_stmt)
            if "armor->mCompoundTag=nullptr;" in line and new_stmt in line:
                head, _ = line.split(new_stmt, 1)
                indent = head[:len(head) - len(head.lstrip())]
                lines[i:i + 1] = [head.rstrip(), indent + new_stmt]
                new_idx.append(i + 1)
            else:
                lines[i] = line
                new_idx.append(i)
            note.append("квалифицировано ItemStackBase::")

    if new_idx and "C2039" not in text:
        i = new_idx[0]
        while i > 0 and lines[i - 1].lstrip().startswith("armor->"):
            i -= 1
        indent = lines[i][:len(lines[i]) - len(lines[i].lstrip())]
        lines[i:i] = [
            indent + "// mValid_DeprecatedSeeComment живёт в ItemStackBase, а ItemStack",
            indent + "// начинается с PAD(24) — квалифицированное имя обязательно,",
            indent + "// иначе MSVC C2039 «не является членом ItemStack».",
        ]
        new_idx = [x + 3 for x in new_idx]

    if len(new_idx) > 1:
        for i in reversed(new_idx[1:]):
            del lines[i]
        note.append("свёрнуты повторы")

    return "\n".join(lines), "; ".join(note)


def _drop_json_catch(text, message):
    import re
    pattern = re.compile(
        r"[ \t]*catch \(const nlohmann::json::exception& e\)[ \t]*\r?\n"
        r"[ \t]*\r?\n?"
        r"[ \t]*\{[ \t]*\r?\n"
        r"[^\n]*" + re.escape(message) + r"[^\n]*\r?\n"
        r"[ \t]*\}[ \t]*\r?\n")
    return pattern.subn("", text)


def fix_manager(text):
    if "catch (const nlohmann::json::exception" not in text:
        return text, "уже исправлено"
    text, n1 = _drop_json_catch(text, "Failed to initialize module")
    text, n2 = _drop_json_catch(text, "Failed to enable/disable module")
    if n1 + n2 == 0:
        return text, "НЕ НАЙДЕНО: catch (const nlohmann::json::exception& e) — пропущено"
    if "C4286" not in text:
        text = text.replace(
            '\t\tcatch (const std::exception& e)\n\t\t{\n'
            '\t\t\tspdlog::error("Failed to enable/disable module',
            "\t\t// nlohmann::json::exception — наследник std::exception, отдельная ветвь\n"
            "\t\t// ниже была недостижима (MSVC C4286). Он ловится этим же catch.\n"
            '\t\tcatch (const std::exception& e)\n\t\t{\n'
            '\t\t\tspdlog::error("Failed to enable/disable module', 1)
    return text, "удалено недостижимых catch: %d" % (n1 + n2)


BUILDINFO_PAIRS = [
    ('    ChatUtils::displayClientMessage("§6- §eIntended Minecraft version§7: {}", '
     'std::string(SOLSTICE_INTENDED_VERSION != "" ? SOLSTICE_INTENDED_VERSION : "N/A"));',
     "    // SOLSTICE_INTENDED_VERSION — строковый литерал, поэтому `!= \"\"` сравнивает\n"
     "    // массивы (MSVC C5056) и всегда истинно. Сравниваем как std::string.\n"
     "    ChatUtils::displayClientMessage(\"§6- §eIntended Minecraft version§7: {}\",\n"
     "        std::string(SOLSTICE_INTENDED_VERSION) != \"\" ? std::string(SOLSTICE_INTENDED_VERSION) "
     ": std::string(\"N/A\"));"),
    ('    ChatUtils::displayClientMessage("§e{}§6 files changed locally compared to the last commit", '
     'STRING(SOLSTICE_FILES_CHANGED_COUNT));',
     "    // SOLSTICE_FILES_CHANGED_COUNT — число, а не строка: STRING() здесь дал бы\n"
     "    // \"123\" как литерал, но без <Utils/StringUtils.hpp> макрос вообще не виден.\n"
     "    // to_string проще и не зависит от порядка включений.\n"
     "    ChatUtils::displayClientMessage(\"§e{}§6 files changed locally compared to the last commit\",\n"
     "        std::to_string(SOLSTICE_FILES_CHANGED_COUNT));"),
]


def fix_buildinfo(text):
    done = []
    for old, new in BUILDINFO_PAIRS:
        if old in text:
            text = text.replace(old, new, 1)
            done.append("правка применена")
    if done:
        return text, "C5056/STRING(): %s" % "; ".join(done)
    if "std::to_string(SOLSTICE_FILES_CHANGED_COUNT)" in text:
        return text, "уже исправлено"
    return text, "НЕ НАЙДЕНО: строки /buildinfo — пропущено"


CMAKE_ANCHOR = ('file(APPEND "${TEMP_BUILD_INFO_PATH}" "#define DEXKO_INTENDED_VERSION '
                '\\"${DEXKO_INTENDED_VERSION}\\"\\n")\n')

CMAKE_BLOCK = (
    "\n# Клиент переименован в Solstice, но генератор по историческим причинам пишет\n"
    "# DEXKO_*. Часть перенесённого кода (ExceptionHandler, BuildInfoCommand) ждёт\n"
    "# SOLSTICE_*, часть (Solstice.cpp) — DEXKO_*, поэтому публикуем ОБА имени с\n"
    "# одинаковыми значениями: одно имя без другого даёт «undefined identifier».\n"
    + "".join(
        'file(APPEND "${TEMP_BUILD_INFO_PATH}" "#define SOLSTICE_%s %s")\n' % (name, value)
        for name, value in [
            ("BUILD_VERSION", '\\"${DEXKO_BUILD_VERSION}\\"\\n'),
            ("BUILD_VERSION_SHORT", '\\"${DEXKO_BUILD_VERSION_SHORT}\\"\\n'),
            ("BUILD_BRANCH", '\\"${DEXKO_BUILD_BRANCH}\\"\\n'),
            ("BUILD_COMMIT_MESSAGE", '\\"${DEXKO_BUILD_COMMIT_MESSAGE}\\"\\n'),
            ("PC_USERNAME", '\\"${DEXKO_PC_USERNAME}\\"\\n'),
            ("INTENDED_VERSION", '\\"${DEXKO_INTENDED_VERSION}\\"\\n'),
            ("FILES_CHANGED_COUNT", "${DEXKO_FILES_CHANGED_COUNT}\\n"),
        ]))


def fix_cmake(text):
    if "SOLSTICE_BUILD_VERSION" in text:
        return text, "уже исправлено"
    if CMAKE_ANCHOR not in text:
        return text, "НЕ НАЙДЕНО: DEXKO_INTENDED_VERSION — пропущено"
    return text.replace(CMAKE_ANCHOR, CMAKE_ANCHOR + CMAKE_BLOCK, 1), "добавлено 7 имён SOLSTICE_*"


def fix_guard(text):
    import re
    if re.search(r"^[ \t]*#[ \t]*pragma[ \t]+once", text, re.M) or \
       re.search(r"^[ \t]*#[ \t]*ifndef[ \t]+\w+", text, re.M):
        return text, "уже есть защита от повторного включения"
    m = re.match(r"((?:[ \t]*//[^\n]*\n|[ \t]*\n)+)", text)
    i = m.end() if m else 0
    return text[:i] + "#pragma once\n\n" + text[i:], "добавлен #pragma once"


PATCH_JOBS = [
    (r"26.62/src/Features/Modules/Player/OreMinerV2.hpp", fix_oreminer),
    (r"26.62/src/Features/Modules/Player/ContainerActions.cpp", fix_container),
    (r"26.62/src/Features/Modules/ModuleManager.cpp", fix_manager),
    (r"26.62/src/Features/Command/Commands/BuildInfoCommand.cpp", fix_buildinfo),
    (r"26.62/GenerateBuildInfo.cmake", fix_cmake),
    (r"26.62/src/Features/Auth/Authorization.hpp", fix_guard),
    (r"26.62/src/Features/Auth/CustomCryptor.hpp", fix_guard),
    (r"26.62/src/Features/Command/Commands/SnipeCommand.hpp", fix_guard),
    (r"26.62/src/Features/Command/Commands/VclipCommand.hpp", fix_guard),
    (r"26.62/src/Features/Events/WindowResizeEvent.hpp", fix_guard),
    (r"26.62/src/SDK/Minecraft/Network/Packets/CameraInstructionPacket.hpp", fix_guard),
]


def run_patch_mode(root, check, backup_dir):
    changed = skipped = problems = 0
    for rel, fn in PATCH_JOBS:
        path = os.path.join(root, rel)
        if not os.path.isfile(path):
            print("  ?  %-64s нет файла" % rel)
            problems += 1
            continue
        with open(path, "rb") as f:
            raw = f.read()
        bom = raw.startswith(b"\xef\xbb\xbf")
        if bom:
            raw = raw[3:]
        text = raw.decode("utf-8")
        crlf = "\r\n" in text
        if crlf:
            text = text.replace("\r\n", "\n")

        new, msg = fn(text)
        if new == text:
            if msg.startswith("НЕ НАЙДЕНО"):
                problems += 1
                print("  !  %-64s %s" % (rel, msg))
            else:
                skipped += 1
                print("  =  %-64s %s" % (rel, msg))
            continue

        changed += 1
        print("  +  %-64s %s" % (rel, msg))
        if check:
            continue
        if backup_dir:
            dst = os.path.join(backup_dir, rel)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(path, dst)
        out = new.replace("\n", "\r\n") if crlf else new
        data = out.encode("utf-8")
        if bom:
            data = b"\xef\xbb\xbf" + data
        with open(path, "wb") as f:
            f.write(data)

    print("\n  изменено: %d, уже было: %d, проблем: %d%s"
          % (changed, skipped, problems, " (--check: ничего не записано)" if check else ""))
    return problems == 0


# ============================================================================

def main():
    ap = argparse.ArgumentParser(
        description="Применить исправления 26.62: из коммита (git) или правками текстом")
    ap.add_argument("--root", default=".", help="корень репозитория (где лежит 26.62/)")
    ap.add_argument("--mode", choices=["auto", "git", "patches"], default="auto",
                    help="git — взять файлы из коммита как есть; patches — правки текстом; "
                         "auto — git, если коммит есть, иначе patches")
    ap.add_argument("--check", action="store_true", help="ничего не писать, только показать")
    ap.add_argument("--no-backup", action="store_true", help="не делать резервную копию")
    args = ap.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isdir(os.path.join(root, "26.62")):
        print("ОШИБКА: в %s нет папки 26.62 — укажи --root на корень репозитория" % root)
        return 2

    repo = is_git_repo(root)
    print("папка: %s" % root)
    print("git-репозиторий: %s" % ("да" if repo else "нет"))
    if repo:
        found = [c for c in COMMITS if have_commit(root, c)]
        print("коммиты найдены: %s" % (", ".join(found) if found else "нет"))

    mode = args.mode
    if mode == "auto":
        mode = "git" if repo and any(have_commit(root, c) for c in COMMITS) else "patches"

    backup_dir = None
    if not args.check and not args.no_backup:
        backup_dir = os.path.join(root, "backup_apply_commit_" + time.strftime("%Y%m%d_%H%M%S"))
        os.makedirs(backup_dir, exist_ok=True)
        print("резервная копия: %s" % backup_dir)

    ok = True
    print("\nрежим: %s" % mode)

    if mode == "git":
        ok = run_git_mode(root, args.check, backup_dir) is not False
        if not ok:
            print("\n  перехожу на правки текстом")
            ok = run_patch_mode(root, args.check, backup_dir)
    else:
        ok = run_patch_mode(root, args.check, backup_dir)

    print("\n" + ("готово" if ok else "есть замечания — смотри строки с ! выше"))
    if not args.check:
        print("проверка после правок: python tools/check_all.sh (bash) или собери проект заново")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
