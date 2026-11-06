#!/usr/bin/env python3
"""
Approximate "does this member exist?" check for ported files.

No compiler is available for this project (MSVC + Windows SDK), so instead of a
real build we build the vocabulary of every identifier declared/used in the new
tree and flag member accesses in the ported files that appear nowhere in it:

    actor->getOffhandContainer()     method access
    stack->mValid                    field access
    Level::getPlayerList             qualified static access

Usage:
    python3 tools/check_members.py 26.62/src/Features/Modules/Combat/*.cpp
    python3 tools/check_members.py --fix-hints 26.62/src/Features/Modules/Combat
"""

import argparse
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
NEW = os.path.join(ROOT, '26.62', 'src')

MEMBER_CALL = re.compile(r'(?:->|\.)\s*([A-Za-z_]\w*)\s*\(')
MEMBER_FIELD = re.compile(r'(?:->|\.)\s*(m[A-Z]\w*)\b')
QUALIFIED = re.compile(r'\b([A-Z]\w+)::([A-Za-z_]\w*)')

# members that legitimately live outside our SDK headers (std, ImGui, win, glm...)

# Члены стандартной библиотеки, которые вызываются через объект (entry.is_regular_file()):
# по тексту не понять тип получателя, поэтому просто знаем их по именам.
STL_MEMBERS = {
    'is_regular_file', 'is_directory', 'is_symlink', 'string', 'path', 'filename',
    'extension', 'parent_path', 'stem', 'exists', 'empty', 'size', 'begin', 'end',
    'push_back', 'emplace_back', 'pop_back', 'insert', 'erase', 'find', 'count',
    'clear', 'reserve', 'resize', 'append', 'substr', 'c_str', 'data', 'length',
    'compare', 'replace', 'front', 'back', 'at', 'contains', 'ends_with',
    'starts_with', 'lower_bound', 'upper_bound', 'try_emplace', 'emplace',
    'lock', 'unlock', 'load', 'store', 'exchange', 'fetch_add', 'fetch_sub',
    # nlohmann::json
    'is_null', 'is_object', 'is_array', 'is_string', 'is_number', 'is_boolean',
    'is_discarded', 'dump', 'get', 'value', 'contains', 'items', 'parse',
    'push_back', 'emplace_back', 'operator[]', 'begin', 'end', 'type', 'size',
}
RE_IGNORE = re.compile(
    r'^(?:'
    r'm[A-Z]\w*$|'            # handled separately (fields)
    r'.*$'
    r')')


def build_vocab(skip_dirs=()):
    words = set()
    skip = tuple(os.path.normpath(os.path.join(NEW, d)) for d in skip_dirs)
    for dirpath, _, files in os.walk(NEW):
        if skip and os.path.normpath(dirpath).startswith(skip):
            continue
        for f in files:
            if not f.endswith(('.hpp', '.cpp', '.h')):
                continue
            try:
                with open(os.path.join(dirpath, f), encoding='utf-8', errors='ignore') as fh:
                    for line in fh:
                        for m in re.finditer(r'[A-Za-z_]\w*', line):
                            w = m.group(0)
                            words.add(w)
                            # split on _ so mBlocksReadPtr also teaches the suffix
                            for part in w.split('_'):
                                words.add(part)
            except OSError:
                pass
    return words


def scan(files, vocab):
    findings = []
    for path in files:
        try:
            lines = open(path, encoding='utf-8', errors='ignore').read().split('\n')
        except OSError:
            continue
        for n, line in enumerate(lines, 1):
            code = line.split('//')[0]
            if not code.strip() or code.lstrip().startswith('*'):
                continue
            for pat, kind in ((MEMBER_CALL, 'method'), (MEMBER_FIELD, 'field')):
                for m in pat.finditer(code):
                    name = m.group(1)
                    if kind == 'method' and (name in vocab or name in STL_MEMBERS):
                        continue
                    # std::filesystem::is_regular_file и прочее — не наш SDK
                    pre = code[max(0, m.start(1) - 8):m.start(1)]
                    if re.search(r'(std|fs|glm|ImGui|WSA)::$', pre):
                        continue
                    if kind == 'field' and name in vocab:
                        continue
                    # skip anything that is a well-known std/ImGui/glm/win call
                    findings.append((path, n, kind, name, line.strip()[:120]))
            for m in QUALIFIED.finditer(code):
                cls, member = m.group(1), m.group(2)
                # Scoped enum-значение (TargetMode::Players, RenderTarget::Players)
                # — это не член SDK, а перечислитель из нашего же модуля.
                if member[:1].isupper() and member.isidentifier():
                    continue
                if member not in vocab and member.isidentifier():
                    findings.append((path, n, f'{cls}::', member, line.strip()[:120]))
    return findings


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('paths', nargs='+')
    ap.add_argument('--exclude-regex', default=None,
                    help='drop findings whose member name matches this regex')
    ap.add_argument('--only-sdkish', action='store_true', default=True,
                    help='report only get/set/is/has/on/mMember style names (default on)')
    ap.add_argument('--vocab-exclude', nargs='*', default=[],
                    help='directories (relative to 26.62/src) left out of the vocabulary, '
                         'e.g. the freshly ported folders')
    a = ap.parse_args()

    files = []
    for p in a.paths:
        if os.path.isdir(p):
            for dirpath, _, fs in os.walk(p):
                files += [os.path.join(dirpath, f) for f in fs if f.endswith(('.cpp', '.hpp'))]
        else:
            files.append(p)

    vocab = build_vocab(a.vocab_exclude)
    findings = scan(files, vocab)
    if a.only_sdkish:
        sdkish = re.compile(r'^(get|set|is|has|on|try|begin|end|m[A-Z]|Player|Actor)')
        findings = [f for f in findings if sdkish.search(f[3])]
    if a.exclude_regex:
        rx = re.compile(a.exclude_regex)
        findings = [f for f in findings if not rx.search(f[3])]

    # A member defined in the same file it is used in is not a portability problem.
    kept = []
    for path, n, kind, name, text in findings:
        # a member declared in the module's own header (or the same file) is fine
        stem = os.path.splitext(path)[0]
        src = ''
        # own file + own header + every header the own header includes (one level):
        # the member may live in Schematic.hpp, CombatantGuiImpl.hpp, ...
        cands = [path, stem + '.hpp', stem + '.h']
        hdr = ''
        # инклюды ищем и в самом .cpp: AutoCosmetic.cpp включает AutoVote.hpp
        for c in (path, stem + '.hpp', stem + '.h'):
            try:
                hdr += open(c, encoding='utf-8', errors='ignore').read()
            except OSError:
                pass
        # the header may sit next to the .cpp (Modules/Player/<name>.hpp) OR one
        # level up when the source lives in a sub-folder -- cover both
        parent = os.path.dirname(os.path.dirname(path))
        # #include <Features/Modules/Misc/AutoVote.hpp> -- путь от корня src
        for inc in re.findall(r'#include\s*[<"]([^>"]+)[>"]', hdr):
            cands.append(os.path.join(NEW, inc))
        for inc in re.findall(r'#include\s*"([^"]+)"', hdr):
            cands.append(os.path.join(parent, inc))
        for inc in re.findall(r'#include\s*"([^"]+)"', hdr):
            cands.append(os.path.join(os.path.dirname(path), inc))
        for cand in cands:
            try:
                src += open(cand, encoding='utf-8', errors='ignore').read()
            except OSError:
                pass
        if re.search(r'\b\w+::' + re.escape(name) + r'\s*\(', src):
            continue
        if re.search(r'\b' + re.escape(name) + r'\s*\([^;]*\)\s*(const\s*)?\{', src):
            continue
        # only a declaration exists (member-pointer in listen<...> + prototype):
        #   void onActorRenderEvent(class ActorRenderEvent& event);
        if re.search(r'\b' + re.escape(name) + r'\s*\([^;{]*\)\s*(const\s*)?;', src):
            continue
        # data member:  std::atomic<bool> mRelocating{false};  /  int mHops = 0;
        # скаляр, массив или инициализатор: int mHops = 0;  /  unsigned char mBuf[13];
        if re.search(r'\b' + re.escape(name) + r'\s*(\[[^\]]*\]|\{[^}]*\}|=[^;]*)?;', src):
            continue
        kept.append((path, n, kind, name, text))
    findings = kept

    # collapse: one line per (file, member) with all hit lines
    grouped = {}
    for path, n, kind, name, text in findings:
        grouped.setdefault((path, kind, name), []).append(n)

    for (path, kind, name), lines in sorted(grouped.items()):
        ln = ','.join(str(x) for x in lines[:8])
        print(f'{os.path.relpath(path, ROOT)}:{ln}  {kind} {name}')
    print(f'\n{len(grouped)} unique unresolved members in {len(files)} files', file=sys.stderr)


if __name__ == '__main__':
    main()
