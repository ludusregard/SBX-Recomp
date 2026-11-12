"""C++ fill tables from sym.py's store list: gen_fill.py ADDR NAME [sym args]"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import sys, sym
def words(off, w, v):
    if w == 8:
        if isinstance(v, int):
            return [(off, 4, v >> 32), (off + 4, 4, v & 0xFFFFFFFF)]
        if isinstance(v, tuple) and v[0] == 'pair':
            return [(off, 4, v[1]), (off + 4, 4, v[2])]
        if isinstance(v, tuple) and v[0] == 'img' and v[2] == 8:
            return [(off, 4, ('img', v[1], 4)), (off + 4, 4, ('img', v[1] + 4, 4))]
        raise SystemExit(f'8-byte {v}')
    return [(off, w, v)]
def key(w, v):
    if isinstance(v, tuple) and v[0] == 'reg' and w == 4 and v[1] in ('r4','r5','r6','r7','r8','r9','r10'):
        return ('kArg', int(v[1][1:]))
    if isinstance(v, tuple) and v[0] == 'ptr' and v[1] == 'obj' and w == 4:
        return ('kSelf', v[2] & 0xFFFFFFFF)
    if isinstance(v, int):
        return ({4: 'kWord', 2: 'kHalf', 1: 'kByte'}[w], v)
    if isinstance(v, tuple) and v[0] == 'img' and w == 4:
        return ('kImage', v[1])
    raise SystemExit(f'value {v} w {w}')
def entry(off, w, v):
    if isinstance(v, int):
        k = {4: 'kWord', 2: 'kHalf', 1: 'kByte'}[w]
        return f'{{{off}, {k}, 0x{v:X}}}'
    if isinstance(v, tuple) and v[0] == 'img' and w == 4:
        return f'{{{off}, kImage, 0x{v[1]:08X}}}'
    raise SystemExit(f'value {v} w {w} at {off}')
def argval(x):
    import re
    if x.startswith("('clobber'"):
        return 'keep'
    m = re.match(r"\('reg', 'r(\d+)'\)", x)
    if m:
        n = int(m.group(1))
        return f'args[{n}]' if 4 <= n <= 10 else ('keep' if n == 3 else None)
    m = re.match(r"\('ptr', 'obj', (-?\d+)\)", x)
    if m:
        return f'object + {m.group(1)}'
    if x.startswith('0x'):
        return f'{x}u'
    m = re.match(r"\[(0x[0-9A-F]+)\]$", x)
    if m:
        return f'guest::LoadU32(base, {m.group(1)})'
    m = re.match(r"\('load', 'obj', (\d+), 4\)$", x)
    if m:
        return f'guest::LoadU32(base, object + {m.group(1)})'
    return None
def fargval(k, x):
    import re
    if x.startswith("('clobber'") or x == f"('freg', '{k}')":
        return 'keep'
    m = re.match(r"\('fload', (4|8), (.*)\)$", x)
    if not m: return None
    w, src = int(m.group(1)), m.group(2)
    mi = re.match(r"\('img', (\d+)(?:, \d+)?\)$", src) or re.match(r"\[(0x[0-9A-F]+)\]$", src)
    mo = re.match(r"\('ptr', 'obj', (-?\d+)\)$", src)
    if mi: addr = f'0x{int(mi.group(1), 0):08X}u'
    elif src.startswith('0x') and False: addr = None
    else:
        # a load through the object pointer
        mo = re.match(r"\('load', 'obj', (\d+), (4|8)\)$", src)
        if not mo: return None
        addr = f'object + {mo.group(1)}'
    if w == 4:
        return f'double(std::bit_cast<float>(guest::LoadU32(base, {addr})))'
    return f'std::bit_cast<double>(REX_LOAD_U64({addr}))'
def fo(o, k):
    return f'0x{o:08X}' if k.startswith('kAbs') else str(o)
def fv(v, k):
    return str(v) if k.endswith('Arg') or k.endswith('Self') else f'0x{v:X}'
def main():
    a, name = sys.argv[1].upper(), sys.argv[2]
    sys.argv = [sys.argv[0], a] + sys.argv[3:]
    import io, contextlib
    with contextlib.redirect_stdout(io.StringIO()):
        R = sym.main()
    if R.failed:
        raise SystemExit(f'FAIL {R.failed}')
    seg = []; segs = []
    for nm, off, w, v, what in R.stores:
        if nm == 'CALL':
            segs.append(seg); segs.append(('CALL', off, v)); seg = []
            continue
        if nm not in ('obj', 'abs'):
            raise SystemExit(f'store to {nm}+{off}')
        for o, ww, vv in words(off, w, v):
            k = key(ww, vv)
            if nm == 'abs':
                k = (k[0].replace('k', 'kAbs', 1), k[1])
            seg.append((o,) + k)
    segs.append(seg)
    n = 0
    body = []
    called = False
    for s in segs:
        if isinstance(s, tuple):
            args = s[2]
            m = __import__('re').match(r"\('ptr', 'obj', (\d+)\)", args['r3'])
            if s[1] == '8258C950' and m and args['r4'] == '0x0' and args['r5'].startswith('0x'):
                body.append(f'guest_memory::Clear(base, object + {m.group(1)}, {int(args["r5"],16)});')
            elif all(argval(args[k]) is not None for k in ('r3','r4','r5','r6','r7','r8')):
                import live as _live
                live = set(_live.reads(s[1])) - {'r3','r4','r5','r6','r7','r8'}
                bad = [k for k in live if k.startswith('f') and fargval(k, args[k]) is None]
                bad += [k for k in ('r9', 'r10') if k in live and argval(args[k]) is None]
                if bad:
                    body.append(f'// CALL sub_{s[1]} unknown {bad} {args}')
                    continue
                for k in sorted(live, key=lambda q: int(q[1:])):
                    if not k.startswith('f'): continue
                    v = fargval(k, args[k])
                    if v != 'keep':
                        body.append(f'ctx.{k}.f64 = {v};')
                for k in ('r3', 'r4', 'r5', 'r6', 'r7', 'r8', 'r9', 'r10'):
                    if k in ('r9', 'r10') and k not in live:
                        continue
                    v = argval(args[k])
                    if v == 'keep' or (not called and v == f'args[{k[1:]}]'):
                        continue
                    body.append(f'ctx.{k}.u64 = {v};')
                body.append(f'sub_{s[1]}(ctx, base);')
                called = True
            else:
                body.append(f'// CALL sub_{s[1]} {args}')
            continue
        if not s: continue
        body.append(f'guest_memory::Apply(base, object, k{name}{n}, args);')
        # runs: same kind and value at a constant stride, in store order
        runs = []
        for o, k, v in s:
            if runs:
                ro, rk, rv, rc, rs = runs[-1]
                if rk == k and rv == v and ((rc == 1 and o > ro) or (rc > 1 and o == ro + rc * rs)):
                    st = o - ro if rc == 1 else rs
                    runs[-1] = (ro, rk, rv, rc + 1, st); continue
            runs.append((o, k, v, 1, 0))
        if any(r[3] > 1 for r in runs):
            ents = [f'{{{fo(o,k)}, {k}, {fv(v,k)}' + (f', {c}, {st}}}' if c > 1 else '}') for o, k, v, c, st in runs]
        else:
            ents = [f'{{{fo(o,k)}, {k}, {fv(v,k)}}}' for o, k, v, c, st in runs]
        print(f'// {len(s)} stores')
        print(f'constexpr Fill k{name}{n}[] = {{')
        for i in range(0, len(ents), 4):
            print('    ' + ', '.join(ents[i:i+4]) + ',')
        print('};'); n += 1
    print('// body:'); [print('  ' + b) for b in body]
    r3 = R.r['r3']
    if isinstance(r3, tuple) and r3[0] == 'ptr' and r3[1] == 'obj':
        print(f'// r3: object + {r3[2]}')
    else:
        print('// r3 =', sym.fmt(r3))
main()
