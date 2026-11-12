"""Copy programs from sym.py: gen_copy.py ADDR NAME. r3..r6 are pointers a3..a6.
Each store must copy a word/half/byte/doubleword from one argument pointer to
another (or be a memcpy call between them). Emits C++ statements."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import sys, re, io, contextlib, sym
a, name = sys.argv[1].upper(), sys.argv[2]
sys.argv = [sys.argv[0], a, '--args=r3=a3,r4=a4,r5=a5,r6=a6'] + sys.argv[3:]
with contextlib.redirect_stdout(io.StringIO()):
    R = sym.main()
if R.failed:
    raise SystemExit(f'FAIL {R.failed}')
ops = []
def src_of(v, w):
    if isinstance(v, tuple) and v[0] == 'load' and v[1].startswith('a') and v[3] == w:
        return [(v[1], v[2], w)]
    if isinstance(v, tuple) and v[0] == 'pair' and w == 8:
        x, y = v[1], v[2]
        if all(isinstance(z, tuple) and z[0] == 'load' and z[3] == 4 for z in (x, y)) and x[1] == y[1] and y[2] == x[2] + 4:
            return [(x[1], x[2], 8)]
    if isinstance(v, tuple) and v[0] == 'load' and v[1].startswith('a') and w == 8 and v[3] == 8:
        return [(v[1], v[2], 8)]
    return None
for nm, off, w, v, what in R.stores:
    if nm == 'CALL':
        t, args = off, v
        if t == '8258CB90':
            m3 = re.match(r"\('ptr', '(a\d)', (-?\d+)\)", args['r3']); m4 = re.match(r"\('ptr', '(a\d)', (-?\d+)\)", args['r4'])
            if m3 and m4 and args['r5'].startswith('0x'):
                ops.append(('memcpy', m3.group(1), int(m3.group(2)), m4.group(1), int(m4.group(2)), int(args['r5'], 16)))
                continue
        raise SystemExit(f'call {t} {args}')
    if not nm.startswith('a'):
        raise SystemExit(f'store to {nm}+{off}')
    s = src_of(v, w)
    if not s:
        raise SystemExit(f'value {v} at {nm}+{off}')
    sn, so, sw = s[0]
    unit = 4 if w == 8 else w
    if ops and ops[-1][0] == 'copy' and ops[-1][1] == nm and ops[-1][3] == sn and ops[-1][2] + ops[-1][5] == off and ops[-1][4] + ops[-1][5] == so and ops[-1][6] == unit:
        o = ops[-1]; ops[-1] = (o[0], o[1], o[2], o[3], o[4], o[5] + w, o[6])
    else:
        ops.append(('copy', nm, off, sn, so, w, unit))
names = {'a3': 'r3', 'a4': 'r4', 'a5': 'r5', 'a6': 'r6'}
out = []
for o in ops:
    if o[0] == 'memcpy':
        out.append(f'guest_memory::Memcpy(ctx, base, {names[o[1]]} + {o[2]}, {names[o[3]]} + {o[4]}, {o[5]});')
    else:
        _, dn, do, sn, so, n, unit = o
        fn = {1: 'CopyBytes', 2: 'CopyHalves', 4: 'CopyWords'}[unit]
        out.append(f'{fn}(base, {names[dn]} + {do}, {names[sn]} + {so}, {n});')
r3 = R.r['r3']
ret = None
if isinstance(r3, tuple) and r3[0] == 'ptr':
    ret = f'{names[r3[1]]} + {r3[2]}'
print('\n'.join(out))
print('// r3:', ret if ret else sym.fmt(r3))
