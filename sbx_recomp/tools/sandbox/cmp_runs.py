"""Streams a difftest binary's output and compares each function's original
and native runs by a hash of their lines (memory, calls, status)."""
import sys, subprocess, hashlib, re
exe, seeds = sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else '4'
p = subprocess.Popen([exe, seeds], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
runs = {}; cur = None
for l in p.stdout:
    if l.startswith('RUN '):
        m = re.match(r'RUN (\w+) seed=(\d+) (\w+) status=(\d+)', l)
        cur = runs[(m.group(1), m.group(2), m.group(3))] = [hashlib.md5(), m.group(4), 0, '']
        continue
    if cur is None: continue
    if l.startswith('REGS'):
        cur[3] = ' '.join(x for x in l.split() if x.startswith(('r3=', 'f1=')))
        continue
    cur[0].update(l.encode()); cur[2] += 1
bad = 0
for a in sorted({k[0] for k in runs}):
    out = []
    for s in sorted({k[1] for k in runs if k[0] == a}):
        o, n = runs.get((a, s, 'orig')), runs.get((a, s, 'native'))
        if not o or not n: out.append(f'seed {s} missing'); continue
        if o[1] != n[1]: out.append(f'seed {s} status orig {o[1]} native {n[1]}')
        elif o[1] == '0' and o[0].digest() != n[0].digest(): out.append(f'seed {s} memory/calls differ ({o[2]} vs {n[2]} lines)')
        elif o[1] == '0' and o[3] != n[3]: out.append(f'seed {s} result: orig {o[3]} native {n[3]}')
    print(a, 'same' if not out else '; '.join(out)); bad += bool(out)
print(bad, 'differ of', len({k[0] for k in runs}))
