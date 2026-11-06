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
    r'dwrite|d2d1|DirectX|cryptopp|libhat|nes|entity|include/|mmsystem|xinput|'
    r'[a-z_]+\.h$)')


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
