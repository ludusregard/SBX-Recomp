#!/bin/bash
# Every override the original files had must still exist somewhere in src/.
cd /home/claude
python3 - <<'PY'
import re, glob, os
pat = re.compile(r'REX_FUNC\((sub_[0-9A-F]{8})\)|^\s*SBX_[A-Z_]+\((?:0x)?([0-9A-F]{8})\b|^\s*SBX_[A-Z_]+\((sub_[0-9A-F]{8})\b|(hooks::Native\w+)', re.M)
def found(root):
    out = set()
    for p in glob.glob(root + '/**/*.*', recursive=True):
        if not p.endswith(('.cpp', '.h')): continue
        for m in pat.finditer(open(p, errors='replace').read()):
            g = [x for x in m.groups() if x][0]
            out.add(g if g.startswith(('sub_', 'hooks')) else 'sub_' + g)
    return out
a = found('workpack/sbx_recomp/src'); b = found('wp/sbx_recomp/src')
print('missing:', sorted(a - b) or 'none', '| new:', sorted(b - a) or 'none')
PY
