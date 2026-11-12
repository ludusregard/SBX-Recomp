"""Builds the index the other tools here read: each function's generated
text by address (generated/default/sbx_recomp.*.cpp), the source files that
override it, and port_native.py's grade for it.

  python index.py [port_report.tsv]

The report is port_native.py's per-address TSV (kind, src, dst, grade,
how); without it the grades are left empty."""
import collections
import csv
import glob
import os
import pickle
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths

gen = {}
for f in glob.glob(os.path.join(paths.GENERATED, 'sbx_recomp.*.cpp')):
    txt = open(f, encoding='utf-8', errors='replace').read()
    idx = [(m.start(), m.group(1))
           for m in re.finditer(r'^DEFINE_REX_FUNC\(sub_([0-9A-F]{8})\)', txt, re.M)]
    for i, (s, a) in enumerate(idx):
        e = idx[i + 1][0] if i + 1 < len(idx) else len(txt)
        gen[a] = txt[s:e]
print('generated functions', len(gen))

ov = collections.defaultdict(list)
for f in glob.glob(os.path.join(paths.SRC, '**', '*.*'), recursive=True):
    if not f.endswith(('.cpp', '.h', '.inc')):
        continue
    t = open(f, errors='replace').read()
    for m in re.finditer(r'REX_FUNC\(sub_([0-9A-F]{8})\)', t):
        ov[m.group(1)].append(os.path.relpath(f, paths.REPO))
print('overridden', len(ov))

grade = {}
if len(sys.argv) > 1:
    with open(sys.argv[1], encoding='utf-8', errors='replace') as f:
        for r in csv.DictReader(f, delimiter='\t'):
            if r['kind'] == 'function':
                grade.setdefault(r['dst'].upper(), (r['grade'], r['how'], r['src']))
print('graded', len(grade))
with open(paths.INDEX, 'wb') as f:
    pickle.dump((gen, dict(ov), grade), f)
print('wrote', paths.INDEX)
