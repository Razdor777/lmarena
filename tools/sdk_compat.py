#!/usr/bin/env python3
"""
SDK compatibility inventory: old base (src/, game 1.21.44) vs new base (26.62/, game 1.26.5x).

Read-only analysis. Produces:
  1. SDK headers that exist only in the old tree (custom SDK extensions we must carry over).
  2. Per SDK header: identifiers removed / added (functions, fields, enums).
  3. For every removed identifier: how many and where it is used outside the SDK
     (modules / hooks / commands / utils) — i.e. real compile breakage after the port.
  4. Risk score per module file, so the port order is data-driven.

Usage:  python3 tools/sdk_compat.py [--old src] [--new 26.62/src] [--out docs/SDK_COMPAT_26.62_RU.md]
"""

import argparse
import os
import re
import sys
from collections import defaultdict

IDENT = re.compile(r'[A-Za-z_][A-Za-z0-9_]{2,}')
# a declaration-ish line: identifier followed by '(' (function) or ';'/'=' (field)
DECL = re.compile(r'\b([A-Za-z_][A-Za-z0-9_]{2,})\s*(?:\(|<|\s*=|\s*;)')
SKIP = {
    'public', 'private', 'protected', 'class', 'struct', 'enum', 'namespace', 'return',
    'static', 'inline', 'virtual', 'const', 'constexpr', 'void', 'bool', 'int', 'float',
    'double', 'char', 'auto', 'unsigned', 'signed', 'long', 'short', 'uint8_t', 'uint16_t',
    'uint32_t', 'uint64_t', 'int8_t', 'int16_t', 'int32_t', 'int64_t', 'size_t', 'std',
    'if', 'else', 'for', 'while', 'switch', 'case', 'sizeof', 'static_assert', 'template',
    'typename', 'using', 'typedef', 'operator', 'new', 'delete', 'this', 'nullptr', 'true',
    'false', 'glm', 'vec2', 'vec3', 'vec4', 'mat4', 'uintptr_t', 'intptr_t', 'byte',
    'pragma', 'include', 'define', 'endif', 'ifdef', 'ifndef', 'memset', 'memcpy', 'string',
}


def sdk_headers(root):
    out = {}
    sdk = os.path.join(root, 'SDK')
    for dirpath, _, files in os.walk(sdk):
        for f in files:
            if not f.endswith(('.hpp', '.h')):
                continue
            p = os.path.join(dirpath, f)
            rel = os.path.relpath(p, sdk)
            out[rel] = p
    return out


API_PREFIX = ('get', 'set', 'is', 'has', 'on', 'try', 'from', 'to', 'use', 'begin', 'end',
              'render', 'draw', 'add', 'remove', 'send', 'read', 'write', 'call', 'create',
              'destroy', 'update', 'init', 'tick', 'serialize', 'deserialize', 'contains',
              'can', 'should', 'was')


def looks_like_api(name):
    """Reject locals/params that leak into headers (item, value, data, result, dir, ...)."""
    if len(name) < 3:
        return False
    if name.isupper():                       # enum constant / macro
        return True
    if name.startswith('m') and len(name) > 2 and name[2].isupper():   # mMember
        return True
    if any(c.isupper() for c in name[1:]):   # camelCase / PascalCase
        return True
    if name.startswith(API_PREFIX):
        return True
    return False


def idents(path):
    """Identifiers that look like API declarations inside a header."""
    got = set()
    try:
        with open(path, 'r', encoding='utf-8', errors='ignore') as fh:
            for line in fh:
                s = line.strip()
                if s.startswith('//') or s.startswith('*'):
                    continue
                # skip obvious statement lines (assignments, returns, control flow)
                if re.match(r'^(return|if|for|while|switch|else|auto\s+\w+\s*=|\w+\s*=\s)', s):
                    continue
                for m in DECL.finditer(line):
                    name = m.group(1)
                    if name in SKIP or not looks_like_api(name):
                        continue
                    got.add(name)
    except OSError:
        pass
    return got


def code_files(root):
    """Non-SDK sources of the old tree that we intend to port."""
    for sub in ('Features', 'Hook', 'Utils'):
        base = os.path.join(root, sub)
        for dirpath, _, files in os.walk(base):
            for f in files:
                if f.endswith(('.cpp', '.hpp')):
                    yield os.path.join(dirpath, f)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--old', default='src')
    ap.add_argument('--new', default='26.62/src')
    ap.add_argument('--out', default=None)
    a = ap.parse_args()

    old_h = sdk_headers(a.old)
    new_h = sdk_headers(a.new)
    common = sorted(set(old_h) & set(new_h))
    only_old = sorted(set(old_h) - set(new_h))
    only_new = sorted(set(new_h) - set(old_h))

    old_ids = {k: idents(v) for k, v in old_h.items()}
    new_ids = {k: idents(v) for k, v in new_h.items()}

    # Global vocabulary of the new SDK: if a name still exists *anywhere* in the new
    # tree, it did not disappear from the API surface (only its owner class may have
    # changed) — those go to a separate "check manually" list instead of the main one.
    new_vocab = set()
    for p in new_h.values():
        try:
            with open(p, 'r', encoding='utf-8', errors='ignore') as fh:
                for line in fh:
                    for m in IDENT.finditer(line):
                        new_vocab.add(m.group(0))
        except OSError:
            pass

    # class/struct/enum names are types, not members — never report them as "removed API"
    type_names = set()
    for p_ in old_h.values():
        try:
            with open(p_, 'r', encoding='utf-8', errors='ignore') as fh:
                for m in re.finditer(r'\b(?:class|struct|enum\s+class|enum)\s+([A-Za-z_]\w*)', fh.read()):
                    type_names.add(m.group(1))
        except OSError:
            pass

    removed = {}      # ident -> [headers]
    relocated = {}    # ident -> [headers]  (still exists somewhere, owner differs)
    added = defaultdict(set)
    for rel in common:
        for x in old_ids[rel] - new_ids[rel]:
            if x in type_names:
                continue
            if x in new_vocab:
                relocated.setdefault(x, []).append(rel)
            else:
                removed.setdefault(x, []).append(rel)
        added[rel] = new_ids[rel] - old_ids[rel]

    # usage map in ported (non-SDK) code
    usage = defaultdict(list)
    rel_usage = defaultdict(list)
    per_file = defaultdict(int)
    all_names = set(removed) | set(relocated)
    if all_names:
        pat = re.compile(r'\b(' + '|'.join(sorted(map(re.escape, all_names), key=len, reverse=True)) + r')\b')
        for p in code_files(a.old):
            try:
                with open(p, 'r', encoding='utf-8', errors='ignore') as fh:
                    for n, line in enumerate(fh, 1):
                        if line.lstrip().startswith('//'):
                            continue
                        for m in pat.finditer(line):
                            name = m.group(1)
                            rel = os.path.relpath(p, a.old)
                            if name in removed:
                                usage[name].append(f'{rel}:{n}')
                            else:
                                rel_usage[name].append(f'{rel}:{n}')
                            per_file[rel] += 1
            except OSError:
                pass

    lines = []
    w = lines.append
    w('# SDK-совместимость: 1.21.44 (корень) → 1.26.5x (`26.62/`)')
    w('')
    w('Сгенерировано `tools/sdk_compat.py` (только чтение, ничего не меняет).')
    w('Сравниваются наборы объявлений в `src/SDK/**` и `26.62/src/SDK/**`; для каждого')
    w('пропавшего имени ниже показано, где оно реально используется вне SDK — это и есть')
    w('список правок при переносе модулей.')
    w('')
    w(f'- SDK-заголовков: старых {len(old_h)}, новых {len(new_h)}, общих {len(common)}')
    w(f'- Только в старой базе: **{len(only_old)}** (наш код в SDK, надо переносить руками)')
    w(f'- Только в новой базе: **{len(only_new)}** (новые фичи базы, идут «бесплатно»)')
    w(f'- Полностью пропавших идентификаторов: **{len(removed)}**')
    w(f'- Из них реально используются в наших модулях/хуках: **{sum(1 for v in usage.values() if v)}**')
    w(f'- Переехавших в другой класс/заголовок (имя живо, хозяин сменился): **{len(relocated)}**')
    w('')

    w('## 1. Заголовки SDK только в старой базе (перенести как есть)')
    w('')
    w('| Заголовок |')
    w('|---|')
    for rel in only_old:
        w(f'| `SDK/{rel}` |')
    w('')

    w('## 2. Заголовки SDK только в новой базе (появились в 26.62)')
    w('')
    w('| Заголовок |')
    w('|---|')
    for rel in only_new:
        w(f'| `SDK/{rel}` |')
    w('')

    def ported_state(use, name):
        """Is the file already ported into 26.62/ and does it still use the old name?"""
        rel = use.rsplit(':', 1)[0]
        dst = os.path.join(a.new, rel)
        if not os.path.exists(dst):
            return '—'
        try:
            body = open(dst, encoding='utf-8', errors='ignore').read()
        except OSError:
            return '?'
        live = '\n'.join(l.split('//')[0] for l in body.splitlines())
        if re.search(r'\b' + re.escape(name) + r'\b', live):
            return '⚠️ ещё использует'
        return '✅ исправлено'

    w('## 3. Пропавшие идентификаторы, которые реально используются')
    w('')
    w('Колонка «статус»: файл уже перенесён в `26.62/` и обращение починено (✅),')
    w('перенесён но ещё использует старое имя (⚠️) или ещё не перенесён (—).')
    w('')
    w('| Идентификатор | Был в | Использований | Статус | Где |')
    w('|---|---|---|---|---|')
    for name, uses in sorted(usage.items(), key=lambda kv: -len(kv[1])):
        if not uses:
            continue
        where = '<br>'.join(f'`{u}`' for u in uses[:6])
        if len(uses) > 6:
            where += f'<br>…ещё {len(uses) - 6}'
        hdrs = '<br>'.join(f'`{h}`' for h in removed[name][:3])
        states = [ported_state(u, name) for u in uses]
        done = states.count('✅ исправлено')
        left = len(states) - done
        status = f'✅ {done}' if left == 0 else (f'⚠️ {left} из {len(states)}' if done else '—')
        w(f'| `{name}` | {hdrs} | {len(uses)} | {status} | {where} |')
    w('')

    w('## 3b. Имена, которые остались в новой базе, но у другого класса')
    w('')
    w('Компилятор поругается только если обращаться к ним через старый тип. Проверять при правке.')
    w('')
    w('| Идентификатор | Был в (старый) | Есть в новой (заголовок) | Использований |')
    w('|---|---|---|---|')
    for name, uses in sorted(rel_usage.items(), key=lambda kv: -len(kv[1])):
        if not uses:
            continue
        owners = [r for r, ids in new_ids.items() if name in ids][:2]
        w(f'| `{name}` | {"<br>".join(f"`{h}`" for h in relocated[name][:2])} | '
          f'{"<br>".join(f"`{o}`" for o in owners) or "—"} | {len(uses)} |')
    w('')

    w('## 4. Пропавшие идентификаторы без использования (можно игнорировать)')
    w('')
    unused = sorted(n for n, u in usage.items() if not u)
    w('`' + '`, `'.join(unused) + '`' if unused else '—')
    w('')

    w('## 5. Риск по файлам (сколько обращений к пропавшему API)')
    w('')
    w('| Файл | Обращений |')
    w('|---|---|')
    for rel, c in sorted(per_file.items(), key=lambda kv: -kv[1]):
        w(f'| `{rel}` | {c} |')
    w('')

    w('## 6. Что добавилось в общих заголовках (куда переезжать)')
    w('')
    for rel in common:
        if not added[rel]:
            continue
        w(f'### `SDK/{rel}`')
        w('')
        w('`' + '`, `'.join(sorted(added[rel])) + '`')
        w('')

    text = '\n'.join(lines) + '\n'
    if a.out:
        os.makedirs(os.path.dirname(a.out), exist_ok=True)
        with open(a.out, 'w', encoding='utf-8') as fh:
            fh.write(text)
        print(f'written {a.out}', file=sys.stderr)
    else:
        sys.stdout.write(text)


if __name__ == '__main__':
    main()
