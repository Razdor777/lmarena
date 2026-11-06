#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Починить сборку 26.62: сама находит копию проекта и правит файлы.

Кладите этот файл куда угодно (например в Downloads) и запускайте:

    python fix_now.py

Скрипт сам:
  * ищет все папки, где есть 26.62\\src\\Features\\Modules\\Player\\OreMinerV2.hpp —
    в текущей папке, рядом с собой, в Загрузках и на Рабочем столе;
  * исправляет КАЖДУЮ найденную копию (даже если их несколько и вы не помните,
    какую собираете);
  * после правки показывает проверку: сколько объявлений DebugRow осталось,
    остались ли строки из ошибок C2011/C2086;
  * пишет отчёт в fix_now_log.txt рядом с собой.

Что правится (5 ошибок + 6 заголовков):
  1. OreMinerV2.hpp      — удаляется ВТОРАЯ секция «Debug dump» (C2011/C2086/C2535)
  2. ContainerActions.cpp — armor->ItemStackBase::mValid_DeprecatedSeeComment (C2039)
  3. ModuleManager.cpp    — убираются недостижимые catch json::exception (C4286)
  4. BuildInfoCommand.cpp — std::string(X) != "" и std::to_string (C5056)
  5. GenerateBuildInfo.cmake — публикуются имена SOLSTICE_*
  6. шесть заголовков     — #pragma once

Git не нужен. Перед правкой файлы копируются в backup_fix_now_<время>\\.
"""

import argparse
import os
import re
import shutil
import sys
import time

OREMINER_REL = os.path.join("26.62", "src", "Features", "Modules", "Player", "OreMinerV2.hpp")

SKIP_DIRS = {"build", ".git", ".vs", "out", "node_modules", "__pycache__", "backup_fix_now",
             "Windows", "Program Files", "Program Files (x86)", "ProgramData",
             "AppData", "$Recycle.Bin", "System Volume Information", "Python311"}


# ============================================================================
# Правки
# ============================================================================

OREMINER_HEADER_MARK = "// Debug dump"
OREMINER_TAIL_MARK = "static const char *oreTypeName(int blockId);"
OREMINER_STALE_MARKS = ("deepslate", "mDebugCsvPath")
OREMINER_ANCHOR = "// Строка «сырого» дампа (Dump Seen): блок, который модуль увидел."
OREMINER_WARNING = [
    "ВАЖНО: эта секция — ЕДИНСТВЕННОЕ объявление дампа в классе.",
    "При слиянии версий она попала в файл дважды (два одинаковых объявления",
    "структуры и методов дампа) — MSVC падал с C2011/C2086/C2535. Вторая",
    "копия удалена; не вставляйте её обратно при следующем слиянии.",
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
        return text, "НЕ НАЙДЕНО: секция «Debug dump»"
    if len(sections) == 1:
        return text, "уже исправлено (секция одна)"

    stale = []
    for start, end in sections:
        body = "\n".join(lines[start:(end + 1 if end is not None else start + 1)])
        if any(m in body for m in OREMINER_STALE_MARKS):
            stale.append((start, end))
    if len(stale) != len(sections) - 1 or any(e is None for _, e in stale):
        return text, "НЕ НАЙДЕНО: неясно, какая секция устаревшая"

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
    return "\n".join(lines), "удалено строк: %d" % removed


def fix_container(text):
    lines = text.split("\n")
    old_stmt = "armor->mValid_DeprecatedSeeComment=false;"
    new_stmt = "armor->ItemStackBase::mValid_DeprecatedSeeComment=false;"
    note = []
    old_idx = [i for i, ln in enumerate(lines) if old_stmt in ln and new_stmt not in ln]
    new_idx = [i for i, ln in enumerate(lines) if new_stmt in ln]
    if not old_idx and not new_idx:
        return text, "НЕ НАЙДЕНО: строка сброса брони"

    for i in reversed(old_idx):
        if new_idx:
            del lines[i]
            note.append("удалена устаревшая строка")
        else:
            line = lines[i].replace(old_stmt, new_stmt)
            if "armor->mCompoundTag=nullptr;" in line:
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
    if not note and lines == text.split("\n"):
        note.append("уже исправлено")
    return "\n".join(lines), "; ".join(note)


def _drop_json_catch(text, message):
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
        return text, "НЕ НАЙДЕНО: catch (const nlohmann::json::exception& e)"
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
    done = 0
    for old, new in BUILDINFO_PAIRS:
        if old in text:
            text = text.replace(old, new, 1)
            done += 1
    if done:
        return text, "исправлено строк: %d" % done
    if "std::to_string(SOLSTICE_FILES_CHANGED_COUNT)" in text:
        return text, "уже исправлено"
    return text, "НЕ НАЙДЕНО: строки /buildinfo"


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
        return text, "НЕ НАЙДЕНО: DEXKO_INTENDED_VERSION"
    return text.replace(CMAKE_ANCHOR, CMAKE_ANCHOR + CMAKE_BLOCK, 1), "добавлено 7 имён SOLSTICE_*"


def fix_guard(text):
    if re.search(r"^[ \t]*#[ \t]*pragma[ \t]+once", text, re.M) or \
       re.search(r"^[ \t]*#[ \t]*ifndef[ \t]+\w+", text, re.M):
        return text, "уже есть защита"
    m = re.match(r"((?:[ \t]*//[^\n]*\n|[ \t]*\n)+)", text)
    i = m.end() if m else 0
    return text[:i] + "#pragma once\n\n" + text[i:], "добавлен #pragma once"


JOBS = [
    (os.path.join("26.62", "src", "Features", "Modules", "Player", "OreMinerV2.hpp"), fix_oreminer),
    (os.path.join("26.62", "src", "Features", "Modules", "Player", "ContainerActions.cpp"), fix_container),
    (os.path.join("26.62", "src", "Features", "Modules", "ModuleManager.cpp"), fix_manager),
    (os.path.join("26.62", "src", "Features", "Command", "Commands", "BuildInfoCommand.cpp"), fix_buildinfo),
    (os.path.join("26.62", "GenerateBuildInfo.cmake"), fix_cmake),
    (os.path.join("26.62", "src", "Features", "Auth", "Authorization.hpp"), fix_guard),
    (os.path.join("26.62", "src", "Features", "Auth", "CustomCryptor.hpp"), fix_guard),
    (os.path.join("26.62", "src", "Features", "Command", "Commands", "SnipeCommand.hpp"), fix_guard),
    (os.path.join("26.62", "src", "Features", "Command", "Commands", "VclipCommand.hpp"), fix_guard),
    (os.path.join("26.62", "src", "Features", "Events", "WindowResizeEvent.hpp"), fix_guard),
    (os.path.join("26.62", "src", "SDK", "Minecraft", "Network", "Packets", "CameraInstructionPacket.hpp"),
     fix_guard),
]


# ============================================================================
# Поиск копий проекта
# ============================================================================

def looks_like_root(path):
    """Корень проекта: есть CMakeLists.txt проекта и наш модуль. Одного файла
    мало — папка резервной копии тоже содержит OreMinerV2.hpp."""
    if not os.path.isfile(os.path.join(path, OREMINER_REL)):
        return False
    if not os.path.isdir(os.path.join(path, "26.62", "src")):
        return False
    return os.path.isfile(os.path.join(path, "26.62", "CMakeLists.txt")) or \
        os.path.isfile(os.path.join(path, "CMakeLists.txt"))


def find_roots(start_dirs, max_depth=6):
    found = []
    seen = set()

    def walk(path, depth):
        try:
            real = os.path.realpath(path)
        except OSError:
            return
        if real in seen or depth > max_depth:
            return
        seen.add(real)
        if looks_like_root(path):
            found.append(path)
            return                                   # внутрь этой копии не лезем
        try:
            entries = sorted(os.listdir(path))
        except (PermissionError, OSError):
            return
        for name in entries:
            if name in SKIP_DIRS or name.startswith(("$", "backup_", "backup_fix_now")):
                continue
            sub = os.path.join(path, name)
            if os.path.isdir(sub) and not os.path.islink(sub):
                walk(sub, depth + 1)

    for start in start_dirs:
        if os.path.isdir(start):
            walk(start, 0)
    return found


def candidate_start_dirs(script_dir):
    home = os.path.expanduser("~")
    out = [os.getcwd(), script_dir]
    for name in ("Downloads", "Desktop", "Documents"):
        out.append(os.path.join(home, name))
    # поднимаемся от текущей папки вверх, но не больше трёх уровней:
    # запуск из подпапки проекта, а не сканирование всего диска
    p = os.path.abspath(os.getcwd())
    for _ in range(4):
        out.append(p)
        parent = os.path.dirname(p)
        if parent == p:
            break
        p = parent
    return out


# ============================================================================
# Применение к одной копии
# ============================================================================

def apply_to_root(root, check, backup_root, log):
    log("")
    log("КОПИЯ: %s" % root)
    changed = skipped = problems = 0

    for rel, fn in JOBS:
        path = os.path.join(root, rel)
        if not os.path.isfile(path):
            log("   ?  %-58s нет файла" % rel)
            problems += 1
            continue
        with open(path, "rb") as f:
            raw = f.read()
        bom = raw.startswith(b"\xef\xbb\xbf")
        if bom:
            raw = raw[3:]
        text = raw.decode("utf-8", errors="replace")
        crlf = "\r\n" in text
        if crlf:
            text = text.replace("\r\n", "\n")

        new, msg = fn(text)
        if new == text:
            if msg.startswith("НЕ НАЙДЕНО"):
                problems += 1
                log("   !  %-58s %s" % (rel, msg))
            else:
                skipped += 1
                log("   =  %-58s %s" % (rel, msg))
            continue

        changed += 1
        log("   +  %-58s %s" % (rel, msg))
        if check:
            continue
        if backup_root:
            dst = os.path.join(backup_root, os.path.relpath(path, root))
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(path, dst)
        out = new.replace("\n", "\r\n") if crlf else new
        data = out.encode("utf-8")
        if bom:
            data = b"\xef\xbb\xbf" + data
        with open(path, "wb") as f:
            f.write(data)

    # проверка результата
    op = os.path.join(root, OREMINER_REL)
    if os.path.isfile(op):
        with open(op, encoding="utf-8", errors="replace") as f:
            final = f.read()
        code_lines = [ln for ln in final.split("\n") if not ln.lstrip().startswith("//")]
        rows = sum(1 for ln in code_lines if re.match(r"\s*struct\s+DebugRow\b", ln))
        stale = sum(1 for ln in code_lines if "mDebugCsvPath" in ln)
        log("   проверка OreMinerV2.hpp: struct DebugRow — %d, mDebugCsvPath — %d  %s"
            % (rows, stale, "OK" if rows == 1 and stale == 0 else "ПРОБЛЕМА"))
        if not check and not (rows == 1 and stale == 0):
            problems += 1

    log("   итог: изменено %d, уже было %d, проблем %d" % (changed, skipped, problems))
    return problems == 0


def main():
    ap = argparse.ArgumentParser(description="Починить сборку 26.62 (сам находит копию проекта)")
    ap.add_argument("--root", help="корень проекта (если известен; иначе автопоиск)")
    ap.add_argument("--check", action="store_true", help="не писать, только показать")
    ap.add_argument("--no-backup", action="store_true")
    args = ap.parse_args()

    lines = []

    def log(s=""):
        print(s)
        lines.append(s)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    log("fix_now.py — правки сборки 26.62")
    log("python: %s" % sys.version.split()[0])
    log("папка скрипта: %s" % script_dir)
    log("текущая папка: %s" % os.getcwd())

    if args.root:
        roots = [os.path.abspath(args.root)] if looks_like_root(args.root) else []
        if not roots:
            log("ОШИБКА: в %s нет 26.62\\src\\...\\OreMinerV2.hpp" % args.root)
            return 2
    else:
        log("ищу копии проекта (может занять несколько секунд)…")
        roots = find_roots(candidate_start_dirs(script_dir))

    if not roots:
        log("")
        log("Копия проекта не найдена. Запустите скрипт из папки проекта, например:")
        log(r'  cd "C:\Users\RAYYONA\Downloads\AyuGram Desktop\1\26.62\2\lmarena-arena-0a525e39-lmarena"')
        log("  python fix_now.py")
        log("или укажите путь: python fix_now.py --root \"C:\\...\\lmarena-arena-0a525e39-lmarena\"")
        return 2

    log("найдено копий: %d" % len(roots))
    for r in roots:
        log("   %s" % r)

    backup_root = None
    if not args.check and not args.no_backup:
        backup_root = os.path.join(script_dir, "backup_fix_now_" + time.strftime("%Y%m%d_%H%M%S"))
        log("резервные копии: %s" % backup_root)

    ok = True
    for r in roots:
        ok = apply_to_root(r, args.check, backup_root, log) and ok

    log("")
    log("ГОТОВО" if ok else "ЕСТЬ ЗАМЕЧАНИЯ — смотрите строки с «!»")
    if not args.check:
        log("Теперь в Visual Studio: Сборка → Пересобрать решение (Rebuild), а не Build.")
    try:
        with open(os.path.join(script_dir, "fix_now_log.txt"), "w", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")
        log("отчёт: %s" % os.path.join(script_dir, "fix_now_log.txt"))
    except OSError:
        pass
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
