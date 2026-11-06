#!/usr/bin/env python3
"""
Проверяет, что тип события, на который подписываются через
  listen<FooEvent, &Class::handler>(this)
виден из файла (объявлен в нём или в любой транзитивно включённой шапке).

Зачем: писать `listen<MouseEvent, ...>` в .cpp, где MouseEvent не объявлен, —
ошибка компиляции. После переноса модулей такие места появлялись пачками,
потому что часть .cpp включала только свой .hpp и надеялась на pch.

Usage: python3 tools/check_event_visibility.py [26.62/src]
"""
import os,re,sys
root = sys.argv[1] if len(sys.argv) > 1 else '26.62/src'
def incs(path):
    out=[]
    if not os.path.exists(path): return out
    d=os.path.dirname(path)
    for m in re.finditer(r'#include\s+[<"]([^>"]+)[>"]', open(path,encoding='utf-8',errors='ignore').read()):
        rel=m.group(1)
        for base in (d,'26.62/src','26.62/include'):
            p=os.path.join(base,rel)
            if os.path.exists(p): out.append(p); break
    return out
def defines(path, typ):
    return re.search(rf'\b(?:class|struct)\s+{typ}\b', open(path,encoding='utf-8',errors='ignore').read()) is not None if os.path.exists(path) else False
def visible(path, typ, seen):
    if path in seen: return False
    seen.add(path)
    if defines(path,typ): return True
    return any(visible(i,typ,seen) for i in incs(path))
bad=[]
for d,_,fs in os.walk(root):
    for f in fs:
        if not f.endswith(('.cpp','.hpp')): continue
        p=os.path.join(d,f)
        t=open(p,encoding='utf-8',errors='ignore').read()
        for typ in set(re.findall(r'(?:listen|deafen)<\s*(\w+)',t)):
            if not visible(p,typ,set()): bad.append((p,typ))
for p,t in bad: print(f'{p:70s} {t}')
print('не видно тип события:',len(bad))
