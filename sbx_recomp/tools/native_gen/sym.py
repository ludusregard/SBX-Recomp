"""Symbolic run of a straight-line-ish SBX function from its generated asm
comments: concrete integers where known, image loads kept as ('img', addr),
argument pointers as ('ptr', name, offset). Prints the stores it makes to
memory that isn't the stack, in order, and r3 at the end.

  sym.py ADDR [--args r3=obj,r4=src,...]
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import re, sys, pickle

gen, ov, grade = paths.load_index()
INLINE_ALL = False
INLINE = set()

def lines_of(a):
    out = []
    for l in gen[a].splitlines():
        s = l.strip()
        m = re.match(r'loc_([0-9A-F]+):', s)
        if m:
            out.append(('label', int(m.group(1), 16)))
        elif s.startswith('// '):
            out.append(('ins', s[3:]))
    return out

M32 = 0xFFFFFFFF

class Fail(Exception):
    pass

def s16(x):
    x &= 0xFFFF
    return x - 0x10000 if x & 0x8000 else x

def add(a, b):
    if isinstance(a, tuple) and a[0] == 'delta':
        a, b = b, a
    if isinstance(b, tuple) and b[0] == 'delta' and isinstance(a, tuple) and a[0] == 'ptr' and a[1] == b[3]:
        return ('ptr', b[1], a[2] + b[2])
    if isinstance(a, int) and isinstance(b, int):
        return (a + b) & 0xFFFFFFFFFFFFFFFF
    if isinstance(a, tuple) and a[0] == 'ptr' and isinstance(b, int):
        return ('ptr', a[1], a[2] + s32(b))
    if isinstance(b, tuple) and b[0] == 'ptr' and isinstance(a, int):
        return ('ptr', b[1], b[2] + s32(a))
    return ('sum', a, b)

def s32(x):
    x &= M32
    return x - (1 << 32) if x & 0x80000000 else x

class Run:
    def __init__(self, args):
        self.r = {f'r{i}': ('reg', f'r{i}') for i in range(32)}
        self.f = {f'f{i}': ('freg', f'f{i}') for i in range(32)}
        self.r['r1'] = ('ptr', 'sp', 0)
        for k, v in args.items():
            self.r[k] = ('ptr', v, 0) if not v.lstrip('-').isdigit() else int(v)
        self.mem = {}  # (name, off) -> word value
        self.stores = []
        self.cr = {}
        self.ctr = None
        self.calls = []

    def addr(self, base, disp):
        b = self.r[base] if base != 'r0' else 0
        return add(b, disp)

    def word_store(self, a, val, width, what):
        if isinstance(a, tuple) and a[0] == 'ptr':
            if a[1] == 'sp':
                self.mem[(a[1], a[2])] = (width, val)
                return
            self.stores.append((a[1], a[2], width, val, what))
            self.mem[(a[1], a[2])] = (width, val)
            return
        if isinstance(a, int):
            self.stores.append(('abs', a & M32, width, val, what))
            return
        raise Fail(f'store to {a}')

    def load(self, a, width):
        if isinstance(a, int):
            return ('img', a & M32, width)
        if isinstance(a, tuple) and a[0] == 'ptr':
            k = (a[1], a[2])
            if k in self.mem and self.mem[k][0] == width:
                return self.mem[k][1]
            if width == 8 and (a[1], a[2]) in self.mem and (a[1], a[2] + 4) in self.mem:
                return ('pair', self.mem[(a[1], a[2])][1], self.mem[(a[1], a[2] + 4)][1])
            if a[1] == 'sp':
                raise Fail(f'stack read {a} unknown')
            return ('load', a[1], a[2], width)
        raise Fail(f'load from {a}')

    def run(self, func, depth=0):
        ins = lines_of(func)
        labels = {v: i for i, (k, v) in enumerate(ins) if k == 'label'}
        pc = 0
        steps = 0
        while pc < len(ins):
            steps += 1
            if steps > 200000:
                raise Fail('too many steps')
            k, x = ins[pc]
            pc += 1
            if k == 'label':
                continue
            j = self.step(x, labels, depth)
            if j == 'ret':
                return
            if j is not None:
                pc = j

    def cmp(self, field, a, b, signed):
        if not (isinstance(a, int) and isinstance(b, int)):
            raise Fail(f'compare {a} {b}')
        if signed:
            a, b = s32(a), s32(b)
        else:
            a, b = a & M32, b & M32
        self.cr[field] = (a < b, a > b, a == b)

    def branch_taken(self, op, field):
        lt, gt, eq = self.cr[field]
        return {'beq': eq, 'bne': not eq, 'blt': lt, 'bgt': gt, 'ble': not gt,
                'bge': not lt}[op]

    def step(self, x, labels, depth):
        r = self.r
        m = re.match(r'(\S+)\s*(.*)', x)
        op, rest = m.group(1), m.group(2)
        a = [s.strip() for s in rest.split(',')] if rest else []
        def mem_operand(s):
            mm = re.match(r'(-?\d+)\((r\d+)\)', s)
            return self.addr(mm.group(2), int(mm.group(1)))
        if op in ('mflr', 'mtlr', 'nop', 'dcbt', 'dcbf'):
            return None
        if re.match(r'(stw|std|stfd|stfs) r?(1[2-9]|2\d|3[01]|f\d+),-\d+\(r1\)', x) and op != 'stwu':
            pass
        if op == 'bl' and re.match(r'0x8258c(5|6)', a[0]):
            return None  # savegpr/restgpr
        if op == 'b' and re.match(r'0x8258c(5|6)', a[0]):
            return 'ret'
        if op == 'stwu':
            r['r1'] = add(r['r1'], int(re.match(r'(-?\d+)', a[1]).group(1)))
            return None
        if op == 'li':
            r[a[0]] = s16(int(a[1])) & 0xFFFFFFFFFFFFFFFF; return None
        if op == 'lis':
            r[a[0]] = (s16(int(a[1])) << 16) & 0xFFFFFFFFFFFFFFFF; return None
        if op == 'addi':
            b = r[a[1]] if a[1] != 'r0' else 0
            r[a[0]] = add(b, s16(int(a[2]))); return None
        if op == 'addis':
            r[a[0]] = add(r[a[1]], s16(int(a[2])) << 16); return None
        if op == 'add':
            r[a[0]] = add(r[a[1]], r[a[2]]); return None
        if op in ('ori', 'oris'):
            v = r[a[1]]
            if not isinstance(v, int): raise Fail('ori sym')
            imm = int(a[2]) << (16 if op == 'oris' else 0)
            r[a[0]] = v | imm; return None
        if op == 'subf':
            x1, x2 = r[a[1]], r[a[2]]  # rD = rB - rA
            if isinstance(x1, int) and isinstance(x2, int):
                r[a[0]] = (x2 - x1) & 0xFFFFFFFFFFFFFFFF
            elif x1[0] == 'ptr' and x2[0] == 'ptr':
                if x1[1] == x2[1]:
                    r[a[0]] = (x2[2] - x1[2]) & 0xFFFFFFFFFFFFFFFF
                else:
                    r[a[0]] = ('delta', x2[1], x2[2] - x1[2], x1[1])
            else:
                raise Fail('subf sym')
            return None
        if op == 'mr':
            r[a[0]] = r[a[1]]; return None
        if op == 'mr.':
            r[a[0]] = r[a[1]]; return None
        if op == 'rotlwi' and a[2] == '0':
            r[a[0]] = r[a[1]]; return None
        if op == 'rlwinm':
            v = r[a[1]]
            if not isinstance(v, int): raise Fail('rlwinm sym')
            sh, mb, me = int(a[2]), int(a[3]), int(a[4])
            v &= M32
            rot = ((v << sh) | (v >> (32 - sh))) & M32 if sh else v
            mask = 0
            for bit in range(32):
                pos = bit
                if (mb <= me and mb <= pos <= me) or (mb > me and (pos >= mb or pos <= me)):
                    mask |= 1 << (31 - pos)
            r[a[0]] = rot & mask; return None
        if op == 'mulli':
            v = r[a[1]]
            if not isinstance(v, int): raise Fail('mulli sym')
            r[a[0]] = (s32(v) * s16(int(a[2]))) & 0xFFFFFFFFFFFFFFFF; return None
        if op in ('addic.', 'addic'):
            v = r[a[1]]
            if not isinstance(v, int): raise Fail('addic sym')
            r[a[0]] = (v + s16(int(a[2]))) & 0xFFFFFFFFFFFFFFFF
            if op == 'addic.':
                self.cmp('cr0', r[a[0]], 0, True)
            return None
        if op == 'mtctr':
            self.ctr = r[a[0]]; return None
        if op == 'bdnz':
            self.ctr = (self.ctr - 1) & M32
            if self.ctr != 0:
                return labels[int(a[0], 16)]
            return None
        if op in ('cmpwi', 'cmplwi', 'cmpw', 'cmplw'):
            field = 'cr0'
            if len(a) == 3:
                field, a = a[0], a[1:]
            v = r[a[0]]
            w = s16(int(a[1])) if op.endswith('i') and op != 'cmplwi' else (int(a[1]) if op == 'cmplwi' else r[a[1]])
            self.cmp(field, v, w, op in ('cmpwi', 'cmpw')); return None
        mb = re.match(r'(beq|bne|blt|bgt|ble|bge)(lr)?$', op)
        if mb:
            field = 'cr0'
            tgt = a
            if a and a[0].startswith('cr'):
                field, tgt = a[0], a[1:]
            if self.branch_taken(mb.group(1), field):
                if mb.group(2):
                    return 'ret'
                return labels[int(tgt[0], 16)]
            return None
        if op == 'b':
            t = int(a[0], 16)
            if t in labels:
                return labels[t]
            # tail call
            self.calls.append(('tail', f'{t:08X}'))
            self.call(f'{t:08X}', depth)
            return 'ret'
        if op == 'bl':
            t = f'{int(a[0], 16):08X}'
            self.calls.append(('call', t))
            self.call(t, depth)
            return None
        if op == 'blr':
            return 'ret'
        if op in ('lwz', 'lbz', 'lhz', 'ld', 'lfs', 'lfd', 'lha'):
            ad = mem_operand(a[1])
            w = {'lwz': 4, 'lbz': 1, 'lhz': 2, 'ld': 8, 'lfs': 4, 'lfd': 8, 'lha': 2}[op]
            v = self.load(ad, w)
            if a[0].startswith('f'):
                self.f[a[0]] = ('fload', w, v)
            else:
                r[a[0]] = v
            return None
        if op in ('stw', 'stb', 'sth', 'std', 'stfs', 'stfd'):
            ad = mem_operand(a[1])
            w = {'stw': 4, 'stb': 1, 'sth': 2, 'std': 8, 'stfs': 4, 'stfd': 8}[op]
            src = (self.f if a[0].startswith('f') else r)[a[0]]
            if isinstance(src, tuple) and src[0] == 'fload':
                if src[1] != w:
                    raise Fail(f'float store width: {x}')
                src = src[2]
            if isinstance(src, int):
                src = src & ((1 << (8 * w)) - 1)
            self.word_store(ad, src, w, op)
            return None
        if op in ('stwx', 'stbx', 'lwzx', 'lbzx'):
            ad = add(r[a[1]], r[a[2]])
            if op.startswith('st'):
                w = 4 if op == 'stwx' else 1
                v = r[a[0]]
                if isinstance(v, int): v &= (1 << (8 * w)) - 1
                self.word_store(ad, v, w, op)
            else:
                r[a[0]] = self.load(ad, 4 if op == 'lwzx' else 1)
            return None
        if op == 'fmr':
            self.f[a[0]] = self.f[a[1]]; return None
        raise Fail(f'unsupported: {x}')

    def is_leaf(self, t):
        body = gen.get(t, '')
        return not re.search(r'// (bl 0x|bctrl|b 0x(?!8258c6))', body.replace('// blr', ''))

    def call(self, t, depth):
        if depth > 6:
            raise Fail('deep')
        if t not in gen:
            raise Fail(f'call to {t}')
        if INLINE_ALL or self.is_leaf(t) or t in INLINE:
            saved_sp = self.r['r1']
            saved = {k: self.r[k] for k in (f'r{i}' for i in range(14, 32))}
            saved_f = {k: self.f[k] for k in (f'f{i}' for i in range(14, 32))}
            self.run(t, depth + 1)
            self.r['r1'] = saved_sp
            self.r.update(saved)
            self.f.update(saved_f)
            return
        args = {k: self.r[k] for k in ('r3', 'r4', 'r5', 'r6', 'r7', 'r8', 'r9', 'r10')}
        args.update({k: self.f[k] for k in (f'f{i}' for i in range(1, 9))})
        self.stores.append(('CALL', t, 0, {k: fmt(v) for k, v in args.items()}, 'call'))
        for k in ('r0','r3','r4','r5','r6','r7','r8','r9','r10','r11','r12'):
            self.r[k] = ('ret', t) if k == 'r3' else ('clobber', t)
        for i in range(14):
            self.f[f'f{i}'] = ('clobber', t)

def fmt(v):
    if isinstance(v, int):
        return f'0x{v:X}'
    if isinstance(v, tuple) and v[0] == 'img':
        return f'[0x{v[1]:08X}]'
    if isinstance(v, tuple) and v[0] == 'pair':
        return f'({fmt(v[1])},{fmt(v[2])})'
    return str(v)

def main():
    global INLINE_ALL
    a = sys.argv[1].upper()
    args = {'r3': 'obj'}
    for x in sys.argv[2:]:
        if x.startswith('--inline='):
            INLINE.update(y.upper() for y in x[9:].split(','))
        if x.startswith('--args='):
            args = dict(kv.split('=') for kv in x[7:].split(','))
    R = Run(args)
    R.failed = None
    try:
        R.run(a)
    except Fail as e:
        R.failed = str(e)
        print('FAIL', e)
    for name, off, w, v, what in R.stores:
        if name == 'CALL':
            print(f'CALL {off} {v}')
        else:
            print(f'{name}+{off} {w} {fmt(v)} {what}')
    print('r3 =', fmt(R.r['r3']), 'calls', R.calls)
    print('f1 =', R.f['f1'])
    return R

if __name__ == '__main__':
    main()
