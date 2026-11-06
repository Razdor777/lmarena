#!/usr/bin/env python3
"""
Ищет обращения к несуществующим значениям перечислений: `Mode::Flareon`, когда
в `enum class Mode` такого элемента нет (симптом переноса — модуль помнит
режим, которого в новой базе не объявили).

Как работает: собирает перечисления из заголовков, которые реально видны из
файла (он сам + транзитивные #include), и сверяет каждое `Enum::Value`.
Код под `#ifdef __PRIVATE_BUILD__` / `#if 0` выбрасывается — в релизной сборке
его нет, и значения оттуда «несуществующими» считать нельзя.

Usage:
    python3 tools/check_enums.py 26.62/src
Код возврата: 0 — чисто, 1 — есть обращения к несуществующим значениям.
"""
import os,re,sys
root=sys.argv[1] if len(sys.argv)>1 else '26.62/src'
if '--' in sys.argv: root=sys.argv[sys.argv.index('--')+1]
def strip_dead_blocks(t):
    """Выбрасывает #ifdef __PRIVATE_BUILD__ / #if 0 ... #endif: в релизе этот код
    не компилируется, и «несуществующего» значения перечисления в нём нет."""
    out=[]
    depth=0
    for line in t.split('\n'):
        head=line.strip()
        if depth==0 and re.match(r'#\s*if(def)?\s+(__PRIVATE_BUILD__|__DEBUG__|0)\b', head):
            depth=1
            out.append('')
            continue
        if depth:
            if re.match(r'#\s*if(def|ndef)?\b', head):
                depth+=1
            elif re.match(r'#\s*endif\b', head):
                depth-=1
                if depth<0: depth=0
            out.append('')
            continue
        out.append(line)
    return '\n'.join(out)

def strip(t):
    t=strip_dead_blocks(t)
    t=re.sub(r'/\*.*?\*/','',t,flags=re.S)
    return re.sub(r'//[^\n]*','',t)
def incs(path):
    out=[]
    if not os.path.exists(path): return out
    d=os.path.dirname(path)
    for rel in re.findall(r'#include\s+[<"]([^>"]+)[>"]', open(path,encoding='utf-8',errors='ignore').read()):
        for base in (d,'26.62/src','26.62/include'):
            p=os.path.join(base,rel)
            if os.path.exists(p): out.append(p); break
    return out
cache={}
def enums_of(path):
    if path in cache: return cache[path]
    cache[path]={}
    if not os.path.exists(path): return cache[path]
    t=strip(open(path,encoding='utf-8',errors='ignore').read())
    for m in re.finditer(r'enum\s+(?:class\s+|struct\s+)?(\w+)?\s*(?::[^{]+)?\{([^{}]*)\}', t):
        name=m.group(1)
        if not name: continue
        vals=set()
        for item in m.group(2).split(','):
            item=item.split('=')[0].strip()
            if re.fullmatch(r'\w+',item): vals.add(item)
        cache[path][name]=vals
    return cache[path]
def visible_enum(path,name,seen=None):
    seen=seen or set()
    if path in seen: return None
    seen.add(path)
    e=enums_of(path)
    if name in e: return e[name]
    for i in incs(path):
        r=visible_enum(i,name,seen)
        if r is not None: return r
    return None
bad=[]
for d,_,fs in os.walk(root):
    for f in fs:
        if not f.endswith(('.hpp','.cpp')): continue
        p=os.path.join(d,f)
        t=strip(open(p,encoding='utf-8',errors='ignore').read())
        for e,v in re.findall(r'\b(\w+)::(\w+)\b', t):
            vals=visible_enum(p,e)
            if vals and v not in vals:
                bad.append((p,e,v,sorted(vals)))
seen=set()
for p,e,v,vals in bad:
    if (p,e,v) in seen: continue
    seen.add((p,e,v))
    print(f'{p:64s} {e}::{v}   есть: {vals[:6]}')
print('реальных подозрений:',len(seen))
sys.exit(1 if seen else 0)
