#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
pdb_dump.py — автономный парсер PDB (MSF 7.0) на чистом Python, без IDA и Windows.

ЧТО ДЕЛАЕТ
  * читает PDB-контейнер (MSF), PDB Info (GUID + Age), таблицу потоков;
  * DBI: список модулей, вклад секций, опциональные заголовки секций (VA секций);
  * символы: S_PUB32 / S_GDATA32 / S_LDATA32 / S_GPROC32 / S_LPROC32 / S_UDT / S_CONSTANT
    -> имена функций и глобалов вместе с RVA;
  * TPI: классы/структуры/объединения с РАЗМЕРАМИ и СМЕЩЕНИЯМИ полей, enum'ы со значениями;
  * выгрузка в JSON, в плоский список "имя + RVA" (для IDA/MakeName), и в C++ заголовок
    со структурами (offset/size), пригодный для проекта.

ЗАЧЕМ ЭТО НУЖНО ЗДЕСЬ
  Символьные данные (LeviLamina bedrock-runtime-data или любой публичный PDB) несут имена
  функций и раскладку классов. Скрипт превращает их в данные, которые можно сверить с
  клиентским RE (offset.txt / IDA_Analysis.hpp / AmbienceHook) и сгенерировать точные
  смещения и сигнатуры вместо угаданных.

ИСПОЛЬЗОВАНИЕ
  python3 tools/pdb_dump.py путь.pdb                       # сводка в stdout
  python3 tools/pdb_dump.py путь.pdb --json out.json       # полный дамп
  python3 tools/pdb_dump.py путь.pdb --names names.txt     # "RVA имя" построчно (для IDA)
  python3 tools/pdb_dump.py путь.pdb --header sdk.hpp --filter "Actor|Player|Level"
  python3 tools/pdb_dump.py путь.pdb --types               # только структуры/классы

ОГРАНИЧЕНИЯ (честно)
  * Если PDB урезан до публичных символов (pdbcopy -p), в нём НЕТ типов — будут только
    имена и RVA. Типы есть только в полном PDB.
  * RVA сами по себе корректны только для того бинарника, из которого PDB собран.
    Для ДРУГОГО билда (например, server PDB против client exe) совпадают только имена,
    а адреса надо искать по сигнатурам/сравнением кода.
"""

from __future__ import annotations

import argparse
import json
import os
import struct
import sys
from collections import OrderedDict

MSF_MAGIC = b"Microsoft C/C++ MSF 7.00\r\n\x1aDS\x00\x00\x00"

# --- DBI / streams -------------------------------------------------------------------
STREAM_PDB_INFO = 1
STREAM_TPI = 2
STREAM_DBI = 3
STREAM_IPI = 4

# --- symbol record types -------------------------------------------------------------
S_PUB32 = 0x110E
S_LDATA32 = 0x110C
S_GDATA32 = 0x110D
S_LPROC32 = 0x110A
S_GPROC32 = 0x1110
S_LPROC32_ID = 0x1146
S_GPROC32_ID = 0x1147
S_UDT = 0x1108
S_CONSTANT = 0x1107
S_OBJNAME = 0x1101
S_THUNK32 = 0x1102
S_LABEL32 = 0x1105

# --- type leaf codes -----------------------------------------------------------------
LF_MODIFIER = 0x1001
LF_POINTER = 0x1002
LF_PROCEDURE = 0x1008
LF_MFUNCTION = 0x1009
LF_ARGLIST = 0x1201
LF_FIELDLIST = 0x1203
LF_BITFIELD = 0x1205
LF_METHODLIST = 0x1206
LF_ARRAY = 0x1503
LF_CLASS = 0x1504
LF_STRUCTURE = 0x1505
LF_UNION = 0x1506
LF_ENUM = 0x1507
LF_ALIAS = 0x150A
LF_INTERFACE = 0x1519
LF_VTSHAPE = 0x000A

LF_MEMBER = 0x150D
LF_STMEMBER = 0x150E
LF_METHOD = 0x150F
LF_NESTTYPE = 0x1510
LF_ONEMETHOD = 0x1511
LF_ENUMERATE = 0x1502
LF_BCLASS = 0x1400
LF_VBCLASS = 0x1401
LF_IVBCLASS = 0x1402
LF_INDEX = 0x1404
LF_VFUNCTAB = 0x1409
LF_FRIENDCLS = 0x140A
LF_PAD0 = 0xF0

# --- numeric leaf codes --------------------------------------------------------------
NUMERIC_CODES = {
    0x8000: ("i8", 1), 0x8001: ("i16", 2), 0x8002: ("u16", 2), 0x8003: ("i32", 4),
    0x8004: ("u32", 4), 0x8005: ("f32", 4), 0x8006: ("f64", 8), 0x8007: ("f80", 10),
    0x8008: ("f128", 16), 0x8009: ("i64", 8), 0x800A: ("u64", 8), 0x800B: ("f48", 6),
    0x800C: ("c32", 8), 0x800D: ("c64", 16), 0x800E: ("c80", 20), 0x800F: ("c128", 32),
}

# SimpleTypeKind из cvinfo.h: код -> (имя, размер в байтах)
SIMPLE_TYPES = {
    0x0003: ("void", 0),
    0x0008: ("HRESULT", 4),
    # "старый" диапазон 0x10..0x24 (используется в некоторых PDB)
    0x0010: ("char", 1), 0x0011: ("short", 2), 0x0012: ("long", 4),
    0x0013: ("long long", 8), 0x0014: ("int128", 16),
    0x0020: ("unsigned char", 1), 0x0021: ("unsigned short", 2),
    0x0022: ("unsigned long", 4), 0x0023: ("unsigned long long", 8),
    0x0024: ("unsigned int128", 16),
    0x0030: ("bool", 1),
    0x0040: ("float", 4), 0x0041: ("double", 8), 0x0042: ("long double", 10),
    0x0043: ("float128", 16), 0x0046: ("float16", 2),
    0x0050: ("complex32", 8), 0x0051: ("complex64", 16),
    0x0052: ("complex80", 20), 0x0053: ("complex128", 32),
    # "новый" диапазон 0x70..0x79 (MASM/VS2015+)
    0x0070: ("char", 1), 0x0071: ("wchar_t", 2), 0x0072: ("int", 4),
    0x0073: ("unsigned int", 4), 0x0074: ("long", 4), 0x0075: ("unsigned long", 4),
    0x0076: ("long long", 8), 0x0077: ("unsigned long long", 8),
    0x0078: ("int128", 16), 0x0079: ("unsigned int128", 16),
}
PRIMITIVES = {code: name for code, (name, _size) in SIMPLE_TYPES.items()}
PRIMITIVE_SIZES = {code: size for code, (_name, size) in SIMPLE_TYPES.items()}

class Reader:
    __slots__ = ("data", "pos", "end")

    def __init__(self, data: bytes, pos: int = 0):
        self.data = data
        self.pos = pos
        self.end = len(data)

    def u8(self):
        v = self.data[self.pos]
        self.pos += 1
        return v

    def u16(self):
        v = struct.unpack_from("<H", self.data, self.pos)[0]
        self.pos += 2
        return v

    def i16(self):
        v = struct.unpack_from("<h", self.data, self.pos)[0]
        self.pos += 2
        return v

    def u32(self):
        v = struct.unpack_from("<I", self.data, self.pos)[0]
        self.pos += 4
        return v

    def i32(self):
        v = struct.unpack_from("<i", self.data, self.pos)[0]
        self.pos += 4
        return v

    def u64(self):
        v = struct.unpack_from("<Q", self.data, self.pos)[0]
        self.pos += 8
        return v

    def bytes(self, n: int):
        v = self.data[self.pos:self.pos + n]
        self.pos += n
        return v

    def cstr(self):
        end = self.data.index(b"\x00", self.pos)
        v = self.data[self.pos:end].decode("utf-8", "replace")
        self.pos = end + 1
        return v

    def numeric(self):
        """NumericLeaf: u16; если старший бит выставлен — это код типа значения."""
        first = self.u16()
        if first & 0x8000 == 0:
            return first
        name, size = NUMERIC_CODES.get(first, ("u32", 4))
        if name in ("i8",):
            return struct.unpack("<b", self.bytes(size))[0]
        if name in ("i16",):
            return struct.unpack("<h", self.bytes(size))[0]
        if name in ("u16",):
            return struct.unpack("<H", self.bytes(size))[0]
        if name in ("i32",):
            return struct.unpack("<i", self.bytes(size))[0]
        if name in ("u32",):
            return struct.unpack("<I", self.bytes(size))[0]
        if name in ("i64",):
            return struct.unpack("<q", self.bytes(size))[0]
        if name in ("u64",):
            return struct.unpack("<Q", self.bytes(size))[0]
        self.bytes(size)
        return None

    def align(self, n: int = 4):
        rem = self.pos % n
        if rem:
            self.pos += n - rem

    @property
    def left(self):
        return self.end - self.pos


def read_numeric(data: bytes, pos: int):
    r = Reader(data, pos)
    value = r.numeric()
    return value, r.pos


class MSF:
    """Контейнер MSF 7.0: суперблок -> список потоков."""

    def __init__(self, data: bytes):
        if not data.startswith(MSF_MAGIC):
            raise ValueError("not an MSF/PDB 7.0 file")
        self.data = data
        fields = struct.unpack_from("<6I", data, 32)
        self.block_size, self.free_block_map, self.num_blocks, self.dir_bytes, _unknown, self.block_map = fields
        if self.block_size in (0, 0xFFFFFFFF) or self.block_size > 0x10000:
            raise ValueError("bad block size %d" % self.block_size)

        dir_blocks = self._read_dir_block_list()
        directory = b"".join(self._block(b) for b in dir_blocks)[:self.dir_bytes]
        self.streams = self._parse_directory(directory)

    def _read_dir_block_list(self):
        count = (self.dir_bytes + self.block_size - 1) // self.block_size
        start = self.block_map * self.block_size
        raw = self.data[start:start + count * 4]
        return list(struct.unpack("<%dI" % count, raw))

    def _block(self, index: int):
        off = index * self.block_size
        return self.data[off:off + self.block_size]

    def _parse_directory(self, directory: bytes):
        num = struct.unpack_from("<I", directory, 0)[0]
        sizes = list(struct.unpack_from("<%dI" % num, directory, 4))
        pos = 4 + num * 4
        streams = []
        for size in sizes:
            nblocks = (size + self.block_size - 1) // self.block_size
            blocks = struct.unpack_from("<%dI" % nblocks, directory, pos) if nblocks else ()
            pos += nblocks * 4
            payload = b"".join(self._block(b) for b in blocks)[:size]
            streams.append(payload)
        return streams

    def stream(self, index: int) -> bytes:
        if 0 <= index < len(self.streams):
            return self.streams[index]
        return b""


class TPI:
    """Type Info Stream: раскладка классов/структур и enum'ов."""

    def __init__(self, data: bytes, pointer_size: int = 8):
        self.records = {}
        self.raw_names = {}
        self.pointer_size = pointer_size
        self._size_cache = {}
        if len(data) < 56:
            return
        r = Reader(data)
        self.version = r.u32()
        header_size = r.u32()
        self.type_index_begin = r.u32()
        self.type_index_end = r.u32()
        self.type_record_bytes = r.u32()
        body_start = header_size
        pos = body_start
        end = min(len(data), body_start + self.type_record_bytes)
        index = self.type_index_begin
        while pos + 4 <= end:
            length, leaf = struct.unpack_from("<HH", data, pos)
            if length < 2:
                break
            payload = data[pos + 4:pos + 2 + length]
            self.records[index] = (leaf, payload)
            pos += 2 + length
            index += 1

    # -- разбор имени типа ------------------------------------------------------------
    def type_name(self, index: int, depth: int = 0) -> str:
        if index is None or depth > 6:
            return "<recursive>"
        if index < 0x1000:
            return PRIMITIVES.get(index & 0x7FF, "T_0x%X" % index)
        rec = self.records.get(index)
        if not rec:
            return "TI_0x%X" % index
        leaf, payload = rec
        if leaf in (LF_CLASS, LF_STRUCTURE, LF_UNION, LF_INTERFACE):
            name, _ = self._aggregate_name(payload, leaf)
            return name or "anon_0x%X" % index
        if leaf == LF_ENUM:
            name = self._enum_info(payload)[2]
            return name or "enum_0x%X" % index
        if leaf == LF_ALIAS:
            r = Reader(payload)
            r.u32()
            return r.cstr()
        if leaf == LF_POINTER:
            r = Reader(payload)
            utype = r.u32()
            attr = r.u32()
            base = self.type_name(utype, depth + 1)
            const = " const" if attr & 0x2 else ""
            return base + const + " *"
        if leaf == LF_MODIFIER:
            r = Reader(payload)
            utype = r.u32()
            mods = r.u16()
            names = {0x1: "const", 0x2: "volatile", 0x4: "unaligned"}
            prefix = " ".join(v for k, v in names.items() if mods & k)
            return (prefix + " " + self.type_name(utype, depth + 1)).strip()
        if leaf == LF_ARRAY:
            r = Reader(payload)
            elem = r.u32()
            r.u32()
            size, _ = read_numeric(payload, r.pos)
            return "%s[%s]" % (self.type_name(elem, depth + 1), size)
        if leaf == LF_PROCEDURE:
            r = Reader(payload)
            ret = r.u32()
            return self.type_name(ret, depth + 1) + " (fn)"
        if leaf == LF_MFUNCTION:
            r = Reader(payload)
            ret = r.u32()
            return self.type_name(ret, depth + 1) + " (member fn)"
        if leaf == LF_BITFIELD:
            r = Reader(payload)
            base = r.u32()
            length = r.u8()
            return "%s : %d" % (self.type_name(base, depth + 1), length)
        return "leaf_0x%X_0x%X" % (leaf, index)

    def _aggregate_name(self, payload: bytes, leaf: int):
        r = Reader(payload)
        r.u16()  # count
        prop = r.u16()
        field = r.u32()
        if leaf in (LF_CLASS, LF_STRUCTURE, LF_INTERFACE):
            r.u32()  # derived
            r.u32()  # vshape
        size, vpos = read_numeric(payload, r.pos)
        r.pos = vpos
        name = r.cstr()
        if prop & 0x80 and r.left > 0:
            try:
                r.cstr()
            except ValueError:
                pass
        return name, size

    def _enum_info(self, payload: bytes):
        """LF_ENUM: count, prop, underlying, field_list, name (НЕ как у класса!)."""
        r = Reader(payload)
        r.u16()
        r.u16()
        underlying = r.u32()
        field = r.u32()
        name = r.cstr() if r.left > 0 else ""
        return underlying, field, name

    def struct_size(self, index: int):
        rec = self.records.get(index)
        if not rec:
            return None
        leaf, payload = rec
        if leaf not in (LF_CLASS, LF_STRUCTURE, LF_UNION, LF_INTERFACE):
            return None
        return self._aggregate_name(payload, leaf)[1]

    # -- разбор полей -----------------------------------------------------------------
    def type_size(self, index: int, depth: int = 0):
        """Размер типа в байтах (насколько его можно вычислить по TPI)."""
        if index is None or depth > 8:
            return 0
        if index in self._size_cache:
            return self._size_cache[index]
        size = 0
        if index < 0x1000:
            size = PRIMITIVE_SIZES.get(index & 0x7FF, 0)
        else:
            rec = self.records.get(index)
            if rec:
                leaf, payload = rec
                try:
                    if leaf == LF_POINTER:
                        size = self.pointer_size
                    elif leaf == LF_MODIFIER:
                        size = self.type_size(struct.unpack_from("<I", payload, 0)[0], depth + 1)
                    elif leaf == LF_BITFIELD:
                        size = self.type_size(struct.unpack_from("<I", payload, 0)[0], depth + 1)
                    elif leaf == LF_ARRAY:
                        elem = struct.unpack_from("<I", payload, 0)[0]
                        r = Reader(payload, 8)
                        count, _ = read_numeric(payload, r.pos)
                        size = (self.type_size(elem, depth + 1) or 0) * (count or 0)
                    elif leaf in (LF_CLASS, LF_STRUCTURE, LF_UNION, LF_INTERFACE):
                        declared = self._aggregate_name(payload, leaf)[1]
                        if isinstance(declared, int) and declared:
                            size = declared
                    elif leaf == LF_ENUM:
                        underlying = self._enum_info(payload)[0]
                        size = self.type_size(underlying, depth + 1)
                    elif leaf in (LF_PROCEDURE, LF_MFUNCTION):
                        size = self.pointer_size
                except (struct.error, ValueError, IndexError):
                    size = 0
        self._size_cache[index] = size
        return size

    def field_list(self, index: int, depth: int = 0):
        """Возвращает (members, bases) для LF_FIELDLIST (с переходом по LF_INDEX)."""
        members, bases = [], []
        seen = set()
        while index and index not in seen and depth < 16:
            seen.add(index)
            rec = self.records.get(index)
            if not rec:
                break
            leaf, payload = rec
            if leaf != LF_FIELDLIST:
                break
            r = Reader(payload)
            next_index = None
            while r.left >= 2:
                start = r.pos
                sub = r.u16()
                if sub == LF_INDEX:
                    r.u16()
                    next_index = r.u32()
                    r.align(4)
                    continue
                try:
                    if sub == LF_MEMBER:
                        attr = r.u16()
                        mtype = r.u32()
                        offset, npos = read_numeric(payload, r.pos)
                        r.pos = npos
                        name = r.cstr()
                        member_size = self.type_size(mtype)
                        members.append({
                            "name": name,
                            "offset": offset,
                            "size": member_size,
                            "type_index": mtype,
                            "type": self.type_name(mtype),
                            "access": {1: "private", 2: "protected", 3: "public"}.get(attr & 7, "?"),
                        })
                    elif sub in (LF_BCLASS, LF_VBCLASS, LF_IVBCLASS):
                        attr = r.u16()
                        btype = r.u32()
                        offset, npos = read_numeric(payload, r.pos)
                        r.pos = npos
                        if sub != LF_BCLASS:
                            vpos, npos = read_numeric(payload, r.pos)
                            r.pos = npos
                            vbo, npos = read_numeric(payload, r.pos)
                            r.pos = npos
                        bases.append({
                            "type_index": btype,
                            "type": self.type_name(btype),
                            "offset": offset,
                            "virtual": sub != LF_BCLASS,
                        })
                    elif sub == LF_STMEMBER:
                        r.u16()
                        mtype = r.u32()
                        name = r.cstr()
                        members.append({"name": name, "offset": None, "static": True,
                                        "type_index": mtype, "type": self.type_name(mtype)})
                    elif sub == LF_ENUMERATE:
                        r.u16()
                        value, npos = read_numeric(payload, r.pos)
                        r.pos = npos
                        name = r.cstr()
                        members.append({"name": name, "value": value})
                    elif sub in (LF_METHOD, LF_ONEMETHOD, LF_NESTTYPE, LF_VFUNCTAB, LF_FRIENDCLS):
                        if sub == LF_METHOD:
                            r.u16()
                            r.u32()
                            name = r.cstr()
                        elif sub == LF_ONEMETHOD:
                            attr = r.u16()
                            r.u32()
                            if (attr & 7) in (4, 6):
                                r.u32()
                            name = r.cstr()
                        elif sub == LF_NESTTYPE:
                            r.u16()
                            r.u32()
                            name = r.cstr()
                        else:
                            r.u16()
                            r.u32()
                            name = "<friend>"
                        members.append({"name": name, "method": True})
                    else:
                        # неизвестный подрекорд: дальше идти нельзя
                        r.pos = start
                        break
                except (struct.error, ValueError, IndexError):
                    r.pos = start
                    break
                r.align(4)
            index = next_index
            depth += 1
        return members, bases

    def dump_aggregates(self, filter_sub: str = None):
        out = []
        for index, (leaf, payload) in self.records.items():
            if leaf not in (LF_CLASS, LF_STRUCTURE, LF_UNION, LF_INTERFACE, LF_ENUM):
                continue
            try:
                if leaf == LF_ENUM:
                    name, size = self._enum_info(payload)[2], None
                else:
                    name, size = self._aggregate_name(payload, leaf)
            except (struct.error, ValueError, IndexError):
                continue
            if filter_sub and filter_sub.lower() not in (name or "").lower():
                continue
            kind = {LF_CLASS: "class", LF_STRUCTURE: "struct", LF_UNION: "union",
                    LF_INTERFACE: "interface", LF_ENUM: "enum"}[leaf]
            entry = OrderedDict()
            entry["kind"] = kind
            entry["name"] = name
            entry["size"] = size
            entry["type_index"] = index
            if leaf == LF_ENUM:
                underlying, field, _name = self._enum_info(payload)
                entry["underlying"] = self.type_name(underlying)
                entry["size"] = self.type_size(underlying)
                values, _ = self.field_list(field)
                entry["values"] = values
            else:
                r = Reader(payload)
                r.u16()
                prop = r.u16()
                field = r.u32()
                entry["field_list_index"] = field
                entry["is_forward_ref"] = bool(prop & 0x80)
                members, bases = self.field_list(field)
                entry["members"] = [m for m in members if m.get("offset") is not None]
                computed = 0
                for member in entry["members"]:
                    if member.get("offset") is not None:
                        computed = max(computed, member["offset"] + (member.get("size") or 0))
                entry["computed_size"] = computed
                if isinstance(entry.get("size"), int) and entry["size"] and computed and computed > entry["size"]:
                    entry["size_warning"] = "computed %d > declared %d" % (computed, entry["size"])
                entry["methods"] = [m["name"] for m in members if m.get("method")]
                entry["bases"] = bases
            out.append(entry)
        out.sort(key=lambda e: (e["kind"], e["size"] if isinstance(e["size"], int) else 0), reverse=True)
        return out


class PDB:
    def __init__(self, path: str):
        with open(path, "rb") as handle:
            data = handle.read()
        self.path = path
        self.msf = MSF(data)
        self.info = self._read_info()
        self.dbi = self._read_dbi()
        pointer_size = 4 if self.dbi.get("machine") == "x86" else 8
        self.tpi = (TPI(self.msf.stream(STREAM_TPI), pointer_size)
                    if len(self.msf.streams) > STREAM_TPI else TPI(b"", pointer_size))
        self.publics = []
        self.functions = []
        self.data_symbols = []
        self.udts = []
        self.constants = []
        self._read_symbols()
        self._read_types()

    # -- PDB Info ---------------------------------------------------------------------
    def _read_info(self):
        data = self.msf.stream(STREAM_PDB_INFO)
        info = {"version": None, "signature": None, "age": None, "guid": None}
        if len(data) >= 28:
            version, signature, age = struct.unpack_from("<3I", data, 0)
            guid = data[12:28]
            info.update({
                "version": version, "signature": signature, "age": age,
                "guid": "%08X-%04X-%04X-%s-%s" % (
                    struct.unpack("<I", guid[0:4])[0], struct.unpack("<H", guid[4:6])[0],
                    struct.unpack("<H", guid[6:8])[0], guid[8:10].hex().upper(), guid[10:16].hex().upper()),
                "guid_hex": guid.hex().upper(),
            })
        return info

    # -- DBI --------------------------------------------------------------------------
    def _read_dbi(self):
        data = self.msf.stream(STREAM_DBI)
        dbi = {"modules": [], "sym_records": None, "globals_stream": None, "publics_stream": None,
               "sections": {}, "section_contributions": []}
        if len(data) < 64:
            return dbi
        (version_signature, version_header, age, global_stream, build_number, public_stream,
         pdb_dll_version, sym_record_stream, pdb_dll_rbld, mod_info_size, section_contrib_size,
         section_map_size, source_info_size, type_server_map_size, mfc_type_server,
         optional_dbg_size, ec_substream_size, flags, machine, _pad) = struct.unpack_from("<iiIHHHHHHiiiiiIiiHHI", data, 0)
        dbi.update({
            "age": age, "machine": "x64" if machine == 0x8664 else "x86" if machine == 0x14C else hex(machine),
            "sym_records": sym_record_stream, "globals_stream": global_stream,
            "publics_stream": public_stream, "flags": flags,
        })
        pos = 64
        mod_end = pos + mod_info_size
        while pos + 64 <= mod_end:
            mod = struct.unpack_from("<IHHIIIIIHHHI", data, pos)
            section_contrib_offset, section_contrib_count = mod[8], mod[9]
            name_off = mod[10] if len(mod) > 10 else 0
            raw_name = cstr_at(data, pos + 64, 256)
            printable = all(32 <= ord(ch) < 127 for ch in raw_name[:64])
            entry = {
                "name": raw_name if printable and raw_name else "<anonymous module>",
                "section_contrib_offset": section_contrib_offset,
                "section_contrib_count": section_contrib_count,
            }
            # длина записи: 64 байта + имена (obj, module)
            nxt = pos + 64
            for _ in range(2):
                end = data.index(b"\x00", nxt)
                nxt = end + 1
            while (nxt - pos) % 4:
                nxt += 1
            pos = nxt
            dbi["modules"].append(entry)
        pos = mod_end
        # section contributions
        contrib_start = pos
        pos += section_contrib_size
        for i in range(0, section_contrib_size, 28):
            chunk = data[contrib_start + i:contrib_start + i + 28]
            if len(chunk) < 28:
                break
            (section, _pad1, offset, size, characteristics, module, _pad2,
             _data_crc, _reloc_crc) = struct.unpack("<hHIIIIHHI", chunk[:28])
            dbi["section_contributions"].append({"section": section - 1, "offset": offset,
                                                 "size": size, "module": module})
        # section map, source info, type server map
        pos += section_map_size + source_info_size + type_server_map_size
        pos += ec_substream_size
        # optional debug header: пары (count, stream[]) в фиксированном порядке.
        # В разных генераторах кодировка отличается (0xFFFF = "нет потока", либо сразу индекс),
        # поэтому после канонического разбора идём запасным путём: перебираем все значения
        # как возможные индексы потоков и берём первый, который похож на таблицу секций.
        if optional_dbg_size >= 2:
            header = data[pos:pos + optional_dbg_size]
            counts = {}
            order = ["FPO", "Exception", "Fixup", "OmapToSrc", "OmapFromSrc", "SectionHdr",
                     "TokenRidMap", "Xdata", "Pdata", "NewFPO", "SectionHdrOrig"]
            p = 0
            for name in order:
                if p + 2 > len(header):
                    break
                count = struct.unpack_from("<H", header, p)[0]
                p += 2
                if count in (0, 0xFFFF) or p + 2 * count > len(header):
                    counts[name] = []
                    continue
                counts[name] = list(struct.unpack_from("<%dH" % count, header, p))
                p += 2 * count
            dbi["optional_streams"] = counts

            candidates = []
            for name in ("SectionHdrOrig", "SectionHdr"):
                candidates += list(counts.get(name, []))
            for i in range(0, len(header) - 1, 2):
                candidates.append(struct.unpack_from("<H", header, i)[0])

            tried = set()
            for index in candidates:
                if index in tried or not (0 < index < len(self.msf.streams)):
                    continue
                tried.add(index)
                sections = self._try_section_headers(self.msf.stream(index))
                if sections:
                    dbi["sections"] = sections
                    dbi["section_header_stream"] = index
                    break
        return dbi

    @staticmethod
    def _try_section_headers(data: bytes):
        """Пробует разобрать поток как массив IMAGE_SECTION_HEADER. Пусто = не он."""
        if not data or len(data) % 40 or len(data) > 40 * 200:
            return {}
        sections = OrderedDict()
        for i in range(len(data) // 40):
            chunk = data[i * 40:(i + 1) * 40]
            name = chunk[0:8].rstrip(b"\x00")
            if not name or any(not (32 <= b < 127) for b in name):
                return {}
            virt_size, virt_addr = struct.unpack_from("<II", chunk, 8)
            if virt_size == 0 and virt_addr == 0:
                return {}
            sections[i] = {"name": name.decode("ascii", "replace"),
                           "virtual_address": virt_addr, "virtual_size": virt_size}
        return sections

    def rva(self, segment: int, offset: int):
        """segment (1-based) + offset -> RVA, если есть заголовки секций."""
        section = self.dbi["sections"].get(segment - 1)
        if not section:
            return None
        return section["virtual_address"] + offset

    # -- символы ----------------------------------------------------------------------
    def _iter_records(self, data: bytes):
        pos = 0
        while pos + 4 <= len(data):
            length, kind = struct.unpack_from("<HH", data, pos)
            if length < 2 or pos + 2 + length > len(data):
                break
            yield kind, data[pos + 4:pos + 2 + length]
            pos += 2 + length

    def _read_symbols(self):
        streams = [self.dbi.get("sym_records"), self.dbi.get("globals_stream"), self.dbi.get("publics_stream")]
        seen_funcs, seen_data = set(), set()
        for stream_index in streams:
            if stream_index is None:
                continue
            try:
                data = self.msf.stream(stream_index)
            except IndexError:
                continue
            for kind, payload in self._iter_records(data):
                try:
                    if kind == S_PUB32:
                        flags, offset, segment = struct.unpack_from("<IIH", payload, 0)
                        name = cstr_at(payload, 10)
                        rva = self.rva(segment, offset)
                        if flags & 0x2:  # function
                            key = (name, rva)
                            if key not in seen_funcs:
                                seen_funcs.add(key)
                                self.functions.append({"name": name, "rva": rva, "segment": segment,
                                                       "offset": offset, "public": True})
                        else:
                            key = (name, rva)
                            if key not in seen_data:
                                seen_data.add(key)
                                self.publics.append({"name": name, "rva": rva, "segment": segment,
                                                     "offset": offset})
                    elif kind in (S_GPROC32, S_LPROC32, S_GPROC32_ID, S_LPROC32_ID):
                        (parent, end, nxt, size, dbg_start, dbg_end, type_index,
                         offset) = struct.unpack_from("<IIIIIIII", payload, 0)
                        segment = struct.unpack_from("<H", payload, 32)[0]
                        name = cstr_at(payload, 35)
                        key = (name, self.rva(segment, offset))
                        if key not in seen_funcs:
                            seen_funcs.add(key)
                            self.functions.append({"name": name, "rva": self.rva(segment, offset),
                                                   "segment": segment, "offset": offset, "size": size,
                                                   "type_index": type_index, "public": False})
                    elif kind in (S_GDATA32, S_LDATA32):
                        type_index, offset, segment = struct.unpack_from("<IIH", payload, 0)
                        name = cstr_at(payload, 10)
                        key = (name, self.rva(segment, offset))
                        if key not in seen_data:
                            seen_data.add(key)
                            self.data_symbols.append({"name": name, "rva": self.rva(segment, offset),
                                                      "segment": segment, "offset": offset,
                                                      "type_index": type_index})
                    elif kind == S_UDT:
                        type_index = struct.unpack_from("<I", payload, 0)[0]
                        self.udts.append({"name": cstr_at(payload, 4), "type_index": type_index})
                    elif kind == S_CONSTANT:
                        type_index = struct.unpack_from("<I", payload, 0)[0]
                        value, npos = read_numeric(payload, 4)
                        self.constants.append({"name": cstr_at(payload, npos), "value": value,
                                               "type_index": type_index})
                except (struct.error, ValueError, IndexError):
                    continue

    def _read_types(self):
        pass  # структуры берутся из self.tpi по запросу


def cstr_at(data: bytes, pos: int, limit: int = 4096) -> str:
    end = data.find(b"\x00", pos, pos + limit)
    if end < 0:
        end = min(len(data), pos + limit)
    return data[pos:end].decode("utf-8", "replace")


# ------------------------------------------------------------------------------------
# ВЫГРУЗКА
# ------------------------------------------------------------------------------------
def summary(pdb: PDB) -> str:
    lines = []
    lines.append("файл: %s (%.1f МБ)" % (pdb.path, os.path.getsize(pdb.path) / 1048576.0))
    lines.append("MSF: block=%d, потоков=%d" % (pdb.msf.block_size, len(pdb.msf.streams)))
    info = pdb.info
    lines.append("PDB Info: version=%s age=%s guid=%s" % (info["version"], info["age"], info["guid"]))
    lines.append("DBI: machine=%s модулей=%d секций=%d" % (pdb.dbi.get("machine"), len(pdb.dbi["modules"]),
                                                           len(pdb.dbi["sections"])))
    lines.append("символов: функций=%d, глобалов=%d, публичных=%d, UDT=%d, констант=%d" % (
        len(pdb.functions), len(pdb.data_symbols), len(pdb.publics), len(pdb.udts), len(pdb.constants)))
    lines.append("типов в TPI: %d" % len(pdb.tpi.records))
    aggs = pdb.tpi.dump_aggregates()
    lines.append("структур/классов/enum: %d" % len(aggs))
    if pdb.dbi["modules"]:
        lines.append("модули (першые 5): %s" % ", ".join(m["name"] for m in pdb.dbi["modules"][:5]))
    return "\n".join(lines)


def make_json(pdb: PDB, filter_sub: str = None):
    aggregates = pdb.tpi.dump_aggregates(filter_sub) if pdb.tpi.records else []
    def flt(items, key="name"):
        if not filter_sub:
            return items
        return [i for i in items if filter_sub.lower() in str(i.get(key, "")).lower()]
    return OrderedDict([
        ("meta", OrderedDict([
            ("file", pdb.path), ("info", pdb.info), ("dbi_age", pdb.dbi.get("age")),
            ("machine", pdb.dbi.get("machine")), ("streams", len(pdb.msf.streams)),
            ("tpi_records", len(pdb.tpi.records)),
        ])),
        ("sections", pdb.dbi["sections"]),
        ("modules", [m["name"] for m in pdb.dbi["modules"]]),
        ("functions", flt(pdb.functions)),
        ("function_publics", flt(pdb.functions, "name")),
        ("publics", flt(pdb.publics)),
        ("data_symbols", flt(pdb.data_symbols)),
        ("udts", flt(pdb.udts)),
        ("constants", flt(pdb.constants)),
        ("types", aggregates),
    ])


def make_names(pdb: PDB, filter_sub: str = None):
    lines = []
    for entry in pdb.functions:
        if entry["rva"] is None:
            continue
        if filter_sub and filter_sub.lower() not in entry["name"].lower():
            continue
        lines.append("%08X %s" % (entry["rva"], entry["name"]))
    for entry in pdb.publics:
        if entry["rva"] is None:
            continue
        if filter_sub and filter_sub.lower() not in entry["name"].lower():
            continue
        lines.append("%08X %s" % (entry["rva"], entry["name"]))
    return "\n".join(sorted(set(lines)))


CPP_TYPE = {
    "void": "void", "char": "char", "unsigned char": "unsigned char", "short": "short",
    "unsigned short": "unsigned short", "int": "int", "unsigned int": "unsigned int",
    "long": "long", "unsigned long": "unsigned long",
    "long long": "long long", "unsigned long long": "unsigned long long",
    "int128": "__int128", "unsigned int128": "unsigned __int128",
    "float": "float", "double": "double", "long double": "long double",
    "float128": "__float128", "float16": "__half", "wchar_t": "wchar_t",
    "bool": "bool", "HRESULT": "long",
}


def make_header(pdb: PDB, filter_sub: str = None, only_with_members: bool = True):
    out = ["#pragma once", "// Автоматически сгенерировано tools/pdb_dump.py из %s" % os.path.basename(pdb.path),
           "// Размеры и смещения — из отладочных типов (PDB). ПРАВИТЬ РУКАМИ БЕССМЫСЛЕННО.",
           "#include <cstdint>", "#include <cstddef>", ""]
    aggregates = pdb.tpi.dump_aggregates(filter_sub)
    for agg in aggregates:
        if agg["kind"] == "enum":
            if not agg.get("values"):
                continue
            out.append("enum class %s : %s {" % (agg["name"], CPP_TYPE.get(agg["underlying"], "int")))
            for value in agg["values"]:
                out.append("    %s = %s," % (value["name"], value.get("value")))
            out.append("};")
            out.append("")
            continue
        members = agg.get("members") or []
        if only_with_members and not members:
            continue
        kind = "struct"
        size = agg["size"] if isinstance(agg["size"], int) else 0
        full_name = agg["name"]
        cpp_name = full_name.split("::")[-1] or "anon"
        out.append("// %s %s, size = 0x%X (%d)" % (agg["kind"], full_name, size, size))
        out.append("%s %s { // size 0x%X" % (kind, cpp_name, size))
        pad_index = 0
        cursor = 0
        for member in sorted(members, key=lambda m: m["offset"] if m["offset"] is not None else 0):
            offset = member.get("offset")
            if offset is None:
                continue
            if offset > cursor:
                out.append("    std::byte _pad%d[0x%X];" % (pad_index, offset - cursor))
                pad_index += 1
                cursor = offset
            out.append("    /*0x%03X*/ %s %s;" % (offset, CPP_TYPE.get(member["type"], member["type"]),
                                                  member["name"]))
            cursor = max(cursor, offset + (member.get("size") or 0))
        if size and cursor < size:
            out.append("    std::byte _padEnd[0x%X];" % (size - cursor))
        out.append("};")
        if size:
            out.append("static_assert(sizeof(%s) == 0x%X, \"%s size mismatch\");" % (cpp_name, size, full_name))
        out.append("")
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(description="Автономный парсер PDB (MSF 7.0) без IDA")
    parser.add_argument("pdb", help="путь к .pdb")
    parser.add_argument("--json", metavar="OUT", help="полный дамп в JSON")
    parser.add_argument("--names", metavar="OUT", help="список 'RVA имя' (для IDA/MakeName)")
    parser.add_argument("--header", metavar="OUT", help="C++ заголовок со структурами")
    parser.add_argument("--types", action="store_true", help="показать структуры/классы в stdout")
    parser.add_argument("--filter", help="подстрока для фильтра имён (регистронезависимо)")
    parser.add_argument("--limit", type=int, default=40, help="сколько строк печатать в сводке")
    args = parser.parse_args()

    pdb = PDB(args.pdb)
    print(summary(pdb))

    if args.json:
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump(make_json(pdb, args.filter), handle, ensure_ascii=False, indent=1)
        print("JSON: %s" % args.json)
    if args.names:
        with open(args.names, "w", encoding="utf-8") as handle:
            handle.write(make_names(pdb, args.filter))
        print("NAMES: %s" % args.names)
    if args.header:
        with open(args.header, "w", encoding="utf-8") as handle:
            handle.write(make_header(pdb, args.filter))
        print("HEADER: %s" % args.header)

    if args.types:
        for agg in pdb.tpi.dump_aggregates(args.filter)[:args.limit]:
            print("\n%s %s (size=%s)" % (agg["kind"], agg["name"], agg["size"]))
            for member in (agg.get("members") or [])[:args.limit]:
                print("   +0x%03X  %-40s %s" % (member.get("offset") or 0, member["type"], member["name"]))
    else:
        print("\nфункции (первые %d):" % args.limit)
        for entry in pdb.functions[:args.limit]:
            rva = "0x%X" % entry["rva"] if entry["rva"] is not None else "seg:%d:%X" % (entry["segment"], entry["offset"])
            print("  %-18s %s" % (rva, entry["name"]))
        if pdb.publics:
            print("\nглобалы (первые %d):" % min(args.limit, len(pdb.publics)))
            for entry in pdb.publics[:args.limit]:
                rva = "0x%X" % entry["rva"] if entry["rva"] is not None else "seg:%d:%X" % (entry["segment"], entry["offset"])
                print("  %-18s %s" % (rva, entry["name"]))


if __name__ == "__main__":
    main()
