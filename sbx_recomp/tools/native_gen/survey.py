import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import sys, subprocess, pickle, re
gen, ov, grade = paths.load_index()
for folder in sys.argv[1:]:
    print('===', folder)
    for l in open(sys.argv[2] if len(sys.argv) > 2 else f'f_{folder}.txt'):
        p = l.split()
        if not p or not re.match(r'[0-9A-F]{8}$', p[0]): continue
        a, g, name = p[0], p[1], p[-2]
        lines = gen[a].count('\n')
        r = subprocess.run([sys.executable, os.path.join(paths.HERE, 'gen_fill.py'),a,'T'],capture_output=True,text=True)
        st = 'GEN' if r.returncode == 0 and '// CALL' not in r.stdout and ('r3:' in r.stdout or "r3 = ('ret'" in r.stdout or 'r3 = 0x' in r.stdout) else ''
        if not st:
            r2 = subprocess.run([sys.executable, os.path.join(paths.HERE, 'gen_copy.py'),a,'X'],capture_output=True,text=True)
            if r2.returncode == 0 and re.search(r'// r3: r\d \+ \d+$', r2.stdout.strip()): st = 'COPY'
        o = 'OVR' if a in ov else ''
        print(f'{a} {g} {lines:5} {st:4} {o:3} {name}')
