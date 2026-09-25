#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
pdb_fetch.py — работа с идентификатором PDB, вшитым в PE (запись RSDS).

ЧТО ДАЁТ ВШИТЫЙ ИДЕНТИФИКАТОР
  В каждом exe/dll лежит структура RSDS: имя PDB, GUID и Age — это «отпечаток», по которому
  сервер символов отдаёт РОВНО ТУ PDB, из которой собран этот файл. Сверка GUID+Age
  — единственный надёжный способ понять, подходит ли PDB к exe (версия/дата тут не указ).

ВОЗМОЖНОСТИ
  1) --info            : показать, какая PDB нужна этому exe (имя, GUID, Age, ссылка на символы)
  2) --check файл.pdb  : сравнить PDB с exe (СОВПАДАЕТ / НЕ СОВПАДАЕТ + диагноз)
  3) --download out.pdb: скачать PDB с сервера символов Microsoft (или своего) по этой RSDS
  4) --scan каталог    : найти все PDB в каталоге и показать, какая подходит к exe

ЗАЧЕМ В ПРОЕКТЕ
  Клиент 1.21.44 — «release» сборка без символов. Если для неё PDB где-то опубликована,
  --download достанет её; если нет — --check покажет это за секунду, и тогда остаётся
  только перенос знаний с другой сборки (см. PDB_SYMBOLS.md и tools/pdb_dump.py).

ИСПОЛЬЗОВАНИЕ
  python3 tools/pdb_fetch.py Minecraft.Windows.exe --info
  python3 tools/pdb_fetch.py Minecraft.Windows.exe --check client.pdb
  python3 tools/pdb_fetch.py Minecraft.Windows.exe --download Minecraft.Windows.pdb
  python3 tools/pdb_fetch.py Minecraft.Windows.exe --scan C:\\symbols

  # свой сервер символов (например, локальный кэш symstore)
  python3 tools/pdb_fetch.py x.exe --download out.pdb --server http://localhost/symbols
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
import urllib.request

DEFAULT_SERVER = "https://msdl.microsoft.com/download/symbols/"
DEBUG_TYPE_CODEVIEW = 2


class PE:
    def __init__(self, path: str):
        with open(path, "rb") as handle:
            self.data = handle.read()
        if self.data[:2] != b"MZ":
            raise ValueError("не PE-файл (нет MZ)")
        self.pe_offset = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[self.pe_offset:self.pe_offset + 4] != b"PE\0\0":
            raise ValueError("не PE-файл (нет PE\\0\\0)")
        coff = self.pe_offset + 4
        (self.machine, self.num_sections, self.timestamp, _sym_ptr, _sym_count,
         self.opt_size, self.characteristics) = struct.unpack_from("<HHIIIHH", self.data, coff)
        self.opt_offset = coff + 20
        magic = struct.unpack_from("<H", self.data, self.opt_offset)[0]
        self.is_pe32_plus = magic == 0x20B
        if magic not in (0x10B, 0x20B):
            raise ValueError("неизвестный формат optional header: 0x%X" % magic)
        dd_offset = self.opt_offset + (112 if self.is_pe32_plus else 96)
        self.image_base = struct.unpack_from("<Q" if self.is_pe32_plus else "<I", self.data,
                                             self.opt_offset + (24 if self.is_pe32_plus else 28))[0]
        self.size_of_image = struct.unpack_from("<I", self.data, self.opt_offset + 56)[0]
        self.data_dirs = []
        for i in range(16):
            rva, size = struct.unpack_from("<II", self.data, dd_offset + i * 8)
            self.data_dirs.append((rva, size))
        self.sections = []
        sect_offset = self.opt_offset + self.opt_size
        for i in range(self.num_sections):
            chunk = self.data[sect_offset + i * 40:sect_offset + (i + 1) * 40]
            name = chunk[0:8].rstrip(b"\x00").decode("ascii", "replace")
            virt_size, virt_addr, raw_size, raw_ptr = struct.unpack_from("<IIII", chunk, 8)
            self.sections.append({"name": name, "virtual_address": virt_addr,
                                  "virtual_size": virt_size, "raw_size": raw_size,
                                  "raw_ptr": raw_ptr})

    def rva_to_offset(self, rva: int):
        for section in self.sections:
            if section["virtual_address"] <= rva < section["virtual_address"] + max(section["virtual_size"],
                                                                                    section["raw_size"]):
                return section["raw_ptr"] + (rva - section["virtual_address"])
        return None

    def codeview(self):
        """Ищет запись CodeView (RSDS) в debug directory. Возвращает dict или None."""
        if not self.data_dirs or len(self.data_dirs) < 7:
            return None
        debug_rva, debug_size = self.data_dirs[6]
        if not debug_rva or not debug_size:
            return None
        offset = self.rva_to_offset(debug_rva)
        if offset is None:
            return None
        for index in range(debug_size // 28):
            chunk = self.data[offset + index * 28: offset + (index + 1) * 28]
            if len(chunk) < 28:
                break
            (_chars, _ts, _maj, _min, kind, size, _rva, raw_ptr) = struct.unpack("<IIHHIIII", chunk)
            if kind != DEBUG_TYPE_CODEVIEW or size < 24:
                continue
            payload = self.data[raw_ptr:raw_ptr + size]
            if payload[:4] == b"RSDS":
                guid = payload[4:20]
                age = struct.unpack_from("<I", payload, 20)[0]
                name = payload[24:].split(b"\x00")[0].decode("utf-8", "replace")
                return {"magic": "RSDS", "guid": guid, "age": age, "name": name,
                        "guid_canonical": uuid_text(guid), "guid_nodash": uuid_nodash(guid)}
            if payload[:4] == b"NB10":
                return {"magic": "NB10", "guid": payload[8:24], "age": struct.unpack_from("<I", payload, 4)[0],
                        "name": payload[24:].split(b"\x00")[0].decode("utf-8", "replace"),
                        "guid_canonical": uuid_text(payload[8:24]), "guid_nodash": uuid_nodash(payload[8:24])}
        return None

    def describe(self):
        machine = {0x8664: "x64", 0x14C: "x86", 0xAA64: "ARM64", 0x1C4: "ARM"}.get(self.machine,
                                                                                   hex(self.machine))
        return "PE: machine=%s, секций=%d, image_base=0x%X, timestamp=0x%X, sections=%s" % (
            machine, self.num_sections, self.image_base, self.timestamp,
            ", ".join(s["name"] for s in self.sections[:8]))


def uuid_text(guid: bytes) -> str:
    return "%08X-%04X-%04X-%s-%s" % (
        struct.unpack("<I", guid[0:4])[0], struct.unpack("<H", guid[4:6])[0],
        struct.unpack("<H", guid[6:8])[0], guid[8:10].hex().upper(), guid[10:16].hex().upper())


def uuid_nodash(guid: bytes) -> str:
    return uuid_text(guid).replace("-", "").upper()


def symbol_url(server: str, cv) -> str:
    return "%s%s/%s%X/%s" % (server.rstrip("/") + "/", cv["name"], cv["guid_nodash"], cv["age"], cv["name"])


def read_pdb_identity(path: str):
    """GUID/Age из самой PDB (без полного разбора: только поток PDB Info)."""
    try:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        from pdb_dump import PDB
        pdb = PDB(path)
        guid_hex = pdb.info.get("guid_hex") or ""
        guid = bytes.fromhex(guid_hex) if guid_hex else b""
        return {"guid": guid, "guid_canonical": pdb.info.get("guid"), "age": pdb.info.get("age"),
                "guid_nodash": uuid_nodash(guid) if guid else ""}
    except Exception as exc:  # noqa: BLE001
        return {"error": str(exc)}


def compare(exe_cv, pdb_identity):
    if not exe_cv:
        return "в exe НЕТ записи RSDS — символы искать не по чему"
    if not pdb_identity or pdb_identity.get("error"):
        return "PDB не читается: %s" % (pdb_identity or {}).get("error")
    if exe_cv["guid_nodash"] == pdb_identity["guid_nodash"] and exe_cv["age"] == pdb_identity["age"]:
        return "СОВПАДАЕТ: это ровно та PDB (GUID=%s, age=%s)" % (exe_cv["guid_canonical"], exe_cv["age"])
    return ("НЕ СОВПАДАЕТ: exe ждёт GUID=%s age=%s, а в PDB GUID=%s age=%s\n"
            "  -> имена/типы могут пригодиться, но адреса (RVA) не переносятся 1:1") % (
        exe_cv["guid_canonical"], exe_cv["age"], pdb_identity.get("guid_canonical"),
        pdb_identity.get("age"))


def main():
    parser = argparse.ArgumentParser(description="RSDS-идентификатор PDB из PE, сверка и загрузка символов")
    parser.add_argument("exe", help="путь к exe/dll")
    parser.add_argument("--info", action="store_true", help="показать требуемую PDB")
    parser.add_argument("--check", metavar="PDB", help="сверить PDB с exe")
    parser.add_argument("--scan", metavar="DIR", help="найти подходящую PDB в каталоге")
    parser.add_argument("--download", metavar="OUT.PDB", help="скачать PDB с сервера символов")
    parser.add_argument("--server", default=DEFAULT_SERVER, help="сервер символов (по умолчанию Microsoft)")
    args = parser.parse_args()

    pe = PE(args.exe)
    print(pe.describe())
    cv = pe.codeview()
    if not cv:
        print("RSDS не найден (exe собран без отладочной информации либо debug directory вырезан)")
        return 1
    print("Нужная PDB : %s" % cv["name"])
    print("GUID       : %s (age %s) [%s]" % (cv["guid_canonical"], cv["age"], cv["magic"]))
    print("Ссылка     : %s" % symbol_url(args.server, cv))

    did_something = False
    if args.check:
        did_something = True
        print("\nСверка с %s:\n  %s" % (args.check, compare(cv, read_pdb_identity(args.check))))

    if args.scan:
        did_something = True
        print("\nПоиск PDB в %s:" % args.scan)
        found = 0
        for root, _dirs, files in os.walk(args.scan):
            for name in files:
                if not name.lower().endswith(".pdb"):
                    continue
                full = os.path.join(root, name)
                identity = read_pdb_identity(full)
                if identity.get("guid_nodash") == cv["guid_nodash"] and identity.get("age") == cv["age"]:
                    print("  ПОДХОДИТ: %s" % full)
                    found += 1
        print("  найдено совпадений: %d" % found)

    if args.download:
        did_something = True
        url = symbol_url(args.server, cv)
        print("\nСкачивание: %s" % url)
        try:
            request = urllib.request.Request(url, headers={"User-Agent": "pdb_fetch/1.0"})
            with urllib.request.urlopen(request, timeout=120) as response, open(args.download, "wb") as out:
                total = 0
                while True:
                    chunk = response.read(1 << 20)
                    if not chunk:
                        break
                    out.write(chunk)
                    total += len(chunk)
            print("  сохранено: %s (%.1f МБ)" % (args.download, total / 1048576.0))
            print("  сверка: %s" % compare(cv, read_pdb_identity(args.download)))
        except Exception as exc:  # noqa: BLE001
            print("  ОШИБКА: %s" % exc)
            print("  Если это 404 — Microsoft не публикует символы для этой сборки "
                  "(для клиента Minecraft это обычная ситуация).")
            return 2

    if not did_something and not args.info:
        print("\n(укажи --check / --scan / --download, чтобы что-то сделать)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
