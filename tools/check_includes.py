#!/usr/bin/env python3
"""
Include sanity check for the 26.62 tree (no compiler needed).

Reports `#include "..."` / `<...>` paths that do not resolve to a file, skipping
commented-out includes and known external dependencies (spdlog, nlohmann, glm,
imgui, entt, cryptopp, libhat, nes, winrt, system headers).

Usage:
    python3 tools/check_includes.py 26.62/src
    python3 tools/check_includes.py --all 26.62/src   # include external deps too
"""

import argparse
import glob
import os
import re
import sys

INC = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
EXTERNAL = re.compile(
    r'^(spdlog|nlohmann|glm|imgui|implot|entt|magic_enum|winrt|Windows|winsock|d3d|dxgi|'
    r'dwrite|d2d1|DirectX|cryptopp|libhat|nes|entity|include/|mmsystem|xinput|kiero|'
    # аудио/midi-заголовки win32, которые не лежат в наших include/
    r'xaudio2|mmdeviceapi|audioclient|wrl|wincodec|shobjidl|propsys|'
    # любые платформенные заголовки вроде DbgHelp.h / ws2tcpip.h / Audio.h:
    # в наших include/ им делать нечего
    r'[A-Za-z0-9_]+\.h$)')


# Заголовки стандартной библиотеки: их нет в наших roots, и это нормально.
STD_HEADERS = {
    'algorithm', 'any', 'array', 'atomic', 'barrier', 'bit', 'bitset', 'cassert',
    'cctype', 'cerrno', 'cfenv', 'cfloat', 'charconv', 'chrono', 'cinttypes',
    'climits', 'clocale', 'cmath', 'codecvt', 'compare', 'complex', 'concepts',
    'condition_variable', 'coroutine', 'csetjmp', 'csignal', 'cstdarg', 'cstddef',
    'cstdint', 'cstdio', 'cstdlib', 'cstring', 'ctime', 'cuchar', 'cwchar',
    'cwctype', 'deque', 'exception', 'execution', 'expected', 'filesystem',
    'format', 'forward_list', 'fstream', 'functional', 'future', 'initializer_list',
    'iomanip', 'ios', 'iosfwd', 'iostream', 'istream', 'iterator', 'latch',
    'limits', 'list', 'locale', 'map', 'mdspan', 'memory', 'memory_resource',
    'mutex', 'new', 'numbers', 'numeric', 'optional', 'ostream', 'queue',
    'random', 'ranges', 'ratio', 'regex', 'scoped_allocator', 'semaphore',
    'set', 'shared_mutex', 'source_location', 'span', 'sstream', 'stack',
    'stdexcept', 'stop_token', 'streambuf', 'string', 'string_view', 'strstream',
    'syncstream', 'system_error', 'thread', 'tuple', 'type_traits', 'typeindex',
    'typeinfo', 'unordered_map', 'unordered_set', 'utility', 'valarray',
    'variant', 'vector', 'version',
    # C-заголовки, которые обычно пишут с .h
    'assert.h', 'ctype.h', 'errno.h', 'float.h', 'limits.h', 'locale.h',
    'math.h', 'setjmp.h', 'signal.h', 'stdarg.h', 'stddef.h', 'stdint.h',
    'stdio.h', 'stdlib.h', 'string.h', 'time.h', 'wchar.h',
}


def resolve(includer, inc, roots):
    cands = [os.path.join(os.path.dirname(includer), inc)]
    cands += [os.path.join(r, inc) for r in roots]
    return any(os.path.exists(c) for c in cands)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('trees', nargs='+', help='source dirs to scan, e.g. 26.62/src')
    ap.add_argument('--roots', nargs='*', default=None,
                    help='extra include roots (default: each tree + 26.62 + 26.62/include)')
    ap.add_argument('--all', action='store_true', help='also report external deps')
    a = ap.parse_args()

    roots = a.roots or sum(([t, os.path.join(os.path.dirname(t), 'include'),
                             os.path.dirname(t)] for t in a.trees), [])

    missing = {}
    for tree in a.trees:
        for f in glob.glob(os.path.join(tree, '**', '*.*'), recursive=True):
            if not f.endswith(('.cpp', '.hpp', '.h', '.inl')):
                continue
            for line in open(f, encoding='utf-8', errors='ignore'):
                m = INC.match(line)
                if not m:
                    continue                      # comments/other lines
                inc = m.group(1)
                if not a.all and inc in STD_HEADERS:
                    continue
                if not a.all and EXTERNAL.match(inc):
                    continue
                if resolve(f, inc, roots):
                    continue
                missing.setdefault(inc, []).append(f)

    for inc, users in sorted(missing.items()):
        print(f'{inc}')
        for u in users[:5]:
            print(f'    <- {u}')
        if len(users) > 5:
            print(f'    ... and {len(users) - 5} more')
    print(f'\n{len(missing)} unresolved include paths', file=sys.stderr)
    return 1 if missing else 0


if __name__ == '__main__':
    sys.exit(main())
