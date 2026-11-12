"""Argument registers (r4-r10, f1-f8) a guest function reads before writing,
by backward dataflow over its blocks. Calls read their callee's arguments and
clobber the volatiles; an indirect call reads all of them."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths
import re, sys
import pickle
gen, ov, grade = paths.load_index()
ARGS = frozenset([f'r{i}' for i in range(3, 11)] + [f'f{i}' for i in range(1, 9)])
VOL = frozenset(['r0'] + [f'r{i}' for i in range(3, 13)] + [f'f{i}' for i in range(14)])
MEMO = {}
NODEF = ('st', 'cmp', 'mt', 'dcb', 'tw', 'b')
def parse(t):
    blocks = [[None, []]]
    for l in gen[t].splitlines():
        s = l.strip()
        m = re.match(r'loc_([0-9A-F]+):', s)
        if m:
            blocks.append([m.group(1), []]); continue
        if s.startswith('// '):
            blocks[-1][1].append(s[3:])
    return blocks
def reads(t, stack=()):
    t = t.upper()
    if t in MEMO: return MEMO[t]
    if t not in gen:
        return frozenset(f'r{i}' for i in range(3, 11))  # an import: integer arguments
    if len(stack) > 40:
        return ARGS
    if t in stack: return frozenset()
    blocks = parse(t)
    labels = {b[0]: i for i, b in enumerate(blocks) if b[0]}
    # split blocks further at branches
    bb = []  # (instrs, succs)
    for bi, (lab, ins) in enumerate(blocks):
        cur = []
        for x in ins:
            cur.append(x)
            op = x.split(' ')[0]
            if op.startswith('b') and op != 'bl' and not op.startswith('bctrl'):
                bb.append(cur); cur = []
        bb.append(cur)
    # rebuild with indices: map label -> first bb index of that block
    bb = []; first = {}
    for bi, (lab, ins) in enumerate(blocks):
        if lab: first[lab] = len(bb)
        cur = []
        for x in ins:
            cur.append(x)
            op = x.split(' ')[0]
            if op.startswith('b') and not op.startswith('bl') and op != 'bctrl':
                bb.append(cur); cur = []
        bb.append(cur)
    n = len(bb)
    use = [set() for _ in range(n)]; dfn = [set() for _ in range(n)]; succ = [[] for _ in range(n)]
    for i, ins in enumerate(bb):
        U, D = use[i], dfn[i]
        fall = True
        for x in ins:
            op, _, rest = x.partition(' ')
            regs = re.findall(r'\b([rf]\d+)\b', rest)
            def u(rs):
                for r in rs:
                    if r not in D: U.add(r)
            m = re.match(r'(bl|b) 0x([0-9a-f]+)$', x)
            if op in ('bl', 'b') and m:
                tgt = m.group(2).upper()
                if op == 'b' and tgt in first:
                    succ[i].append(first[tgt]); fall = False; continue
                if tgt.startswith('8258C6') or tgt.startswith('8258C5'):
                    continue  # save/restore helpers
                u(reads(tgt, stack + (t,)))
                if op == 'b': fall = False
                else: D |= VOL
                continue
            if op in ('bctrl',):
                u(ARGS); D |= VOL; continue
            if op in ('bctr',):
                u(ARGS); fall = False; continue
            if op == 'blr' or op.startswith('blr'):
                if op == 'blr': fall = False
                continue
            if op.startswith('b'):
                mm = re.search(r'0x([0-9a-f]+)$', x)
                if mm and mm.group(1).upper() in first:
                    succ[i].append(first[mm.group(1).upper()])
                elif mm:
                    u(reads(mm.group(1).upper(), stack + (t,)))  # conditional tail call
                continue
            if op.startswith(NODEF) or op in ('rlwimi',):
                u(regs)
                if op == 'rlwimi' and regs: D.add(regs[0])
                if op.startswith(('stwu', 'stdu')) and len(regs) > 1: D.add(regs[-1])
                continue
            if op.startswith(('lwzu', 'lbzu', 'lhzu', 'ldu', 'lfsu', 'lfdu')):
                u(regs[1:]); D.update(regs[:1] + regs[-1:]); continue
            u(regs[1:])
            if regs: D.add(regs[0])
        if fall and i + 1 < n:
            succ[i].append(i + 1)
    live = [set() for _ in range(n)]
    changed = True
    while changed:
        changed = False
        for i in reversed(range(n)):
            out = set()
            for s_ in succ[i]: out |= live[s_]
            new = use[i] | (out - dfn[i])
            if new != live[i]: live[i] = new; changed = True
    res = frozenset(live[0] & ARGS)
    MEMO[t] = res
    return res
if __name__ == '__main__':
    for a in sys.argv[1:]:
        print(a, sorted(reads(a), key=lambda q: (q[0], int(q[1:]))))
