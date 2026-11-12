"""Lists the overrides whose original has a float compare (fcmpu), so each
one's branches can be read against the C++.

  python check_float_compares.py

The generated code's branches on a float compare: blt and bgt are taken
only when lower or greater; ble and bge test "not greater" and "not less",
so both are taken on an unordered compare (a NaN). In C++: ble is
!(a > b), bge is !(a < b)."""
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths

gen, ov, grade = paths.load_index()
for f in sorted(glob.glob(os.path.join(paths.SRC, '**', '*.cpp'), recursive=True)):
    s = open(f, errors='replace').read()
    addrs = set(re.findall(r'REX_FUNC\(sub_([0-9A-F]{8})\)', s))
    addrs |= set(re.findall(r'^SBX_[A-Z_0-9]+\(([0-9A-F]{8})', s, re.M))
    hits = [a for a in sorted(addrs) if a in gen and '// fcmp' in gen[a]]
    if hits:
        print(os.path.relpath(f, paths.SRC), ' '.join(hits))
