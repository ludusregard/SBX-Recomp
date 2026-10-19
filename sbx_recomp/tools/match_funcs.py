#!/usr/bin/env python3
"""Match functions between two ReXGlue generated trees (e.g. another build of the
game -> this one).

Two builds of a game share most code at other addresses. Each generated function
carries its PowerPC instructions as comments ("// lwz r11,9608(r30)"); this
compares those, with addresses and large immediates masked, in passes:

  0. pinned: hand-checked pairs from match_pins.tsv (next to this script).
  1. exact:  the masked instruction sequence is unique on both sides and equal.
     shape:  the same, with registers renamed by first use (library code
             built with another register allocation).
  2. calls:  for two matched functions whose call lists have the same length,
             callees in the same position are paired when their masked code
             is similar enough (repeated until nothing changes).
  3. order:  an unmatched function lying between two matched neighbors on
             both sides, compared with the candidates between them.
  3b. gap:   exactly one unmatched function on each side between adjacent
             matches, with a similar mix of instructions.
  4. local:  a non-exact match whose shift (dst - src) is far from the
             median shift of the nearest exact matches is re-placed by the
             best similarity (plain or register-renamed) within 0x800 of
             the predicted address.
  5. wanted:  addresses asked for explicitly (--want) still unmatched get the
             local search, then the best similarity over all unmatched
             functions ("fuzzy"), reported with its score.
  6. target: a small function whose calls/tail calls don't land on the
             matches of its source's targets moves to an identical candidate
             nearby whose targets do (e.g. three identical 4-instruction
             thunks differing only in where they branch).

Output (TSV): src_addr, dst_addr, method, score, src_len, dst_len, shift_ok
(whether dst - src is within 0x4000 of the local shift), calls_agree (of the
calls/tail calls to matched functions, how many land on their matches).

  match_funcs.py <src generated dir> <dst generated dir> -o matches.tsv
                 [--want 0x8218E420,0x82107B90 ...] [--show 0x8218E420]

--show prints the two instruction listings side by side for a match.
"""
import argparse
import bisect
import collections
import difflib
import glob
import hashlib
import os
import re
import sys

FUNC_RE = re.compile(r'^DEFINE_REX_FUNC\(sub_([0-9A-F]{8})\)')
INSN_RE = re.compile(r'^\t// ([a-z][a-z0-9.+\-]*)(?: (\S+))?\s*$')
NUM_RE = re.compile(r'-?0x[0-9a-fA-F]+|-?\b\d+\b')
BRANCH = re.compile(r'^b[a-z]*$')


def mask(mn, ops):
    """Masked form of one instruction: keep registers and small numbers
    (struct offsets, constants), drop addresses and global offsets."""
    if not ops:
        return mn
    if BRANCH.match(mn) or mn in ('lis',):
        return mn + ' ' + NUM_RE.sub('#', ops)

    def num(m):
        t = m.group(0)
        v = int(t, 16) if 'x' in t else int(t)
        return t if -0x1000 < v < 0x1000 else '#'
    return mn + ' ' + NUM_RE.sub(num, ops)


REG_RE = re.compile(r'\b([rfv])(\d+)\b')


def shape(masked):
    """Masked instructions with registers renamed in order of first use, so
    the same code with another register allocation compares equal. r1 (the
    stack) and r3-r10/f1-f13 as arguments are kept: renaming them would hide
    real differences in what a function takes."""
    names = {}
    out = []
    for ins in masked:
        def ren(m):
            k = m.group(0)
            if k in ('r1', 'r2', 'r13') or (m.group(1) == 'r' and 3 <= int(m.group(2)) <= 10
                                             and k not in names and not out):
                return k
            if k not in names:
                names[k] = f'{m.group(1)}#{len([n for n in names if n[0] == m.group(1)])}'
            return names[k]
        out.append(REG_RE.sub(ren, ins))
    return out


def load(gen_dir):
    """{addr: (masked insns, raw insns, [call targets])}"""
    funcs = {}
    for path in sorted(glob.glob(os.path.join(gen_dir, '*.cpp'))):
        cur = None
        with open(path, encoding='utf-8', errors='replace') as f:
            for line in f:
                m = FUNC_RE.match(line)
                if m:
                    cur = int(m.group(1), 16)
                    funcs[cur] = ([], [], [])
                    continue
                if cur is None:
                    continue
                if line.startswith('}'):
                    cur = None
                    continue
                m = INSN_RE.match(line)
                if not m:
                    continue
                mn, ops = m.group(1), m.group(2) or ''
                masked, raw, calls = funcs[cur]
                masked.append(mask(mn, ops))
                raw.append(f'{mn} {ops}'.strip())
                if mn == 'bl':
                    try:
                        calls.append(int(ops, 16))
                    except ValueError:
                        pass
    return funcs


def digest(seq):
    return hashlib.sha1('\n'.join(seq).encode()).hexdigest()


def similarity(a, b):
    if not a or not b:
        return 0.0
    if abs(len(a) - len(b)) > max(len(a), len(b)) * 0.5:
        return 0.0
    return difflib.SequenceMatcher(None, a, b, autojunk=False).ratio()


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('src')
    ap.add_argument('dst')
    ap.add_argument('-o', '--out', default='matches.tsv')
    ap.add_argument('--want', default='', help='comma-separated src addresses to force-search')
    ap.add_argument('--show', default='', help='comma-separated src addresses to print side by side')
    ap.add_argument('--min-call-score', type=float, default=0.6)
    ap.add_argument('--pins', default=os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                                    'match_pins.tsv'),
                    help='hand-checked matches applied first (default: match_pins.tsv here)')
    args = ap.parse_args()

    src, dst = load(args.src), load(args.dst)
    print(f'functions: src {len(src)}, dst {len(dst)}', file=sys.stderr)
    match = {}   # src -> (dst, method, score)
    used = set()

    def pair(s, d, method, score):
        if s in match or d in used:
            return False
        match[s] = (d, method, score)
        used.add(d)
        return True

    # 0. pins: hand-checked matches (src<TAB>dst<TAB>note; # comments)
    if args.pins and os.path.exists(args.pins):
        for line in open(args.pins):
            line = line.split('#', 1)[0].strip()
            if line:
                a, b = line.split()[:2]
                pair(int(a, 16), int(b, 16), 'pinned', 1.0)
        print(f'pinned: {len(match)}', file=sys.stderr)

    # 1. exact, unique on both sides
    by_s = collections.defaultdict(list)
    by_d = collections.defaultdict(list)
    for a, (m, _, _) in src.items():
        by_s[digest(m)].append(a)
    for a, (m, _, _) in dst.items():
        by_d[digest(m)].append(a)
    for h, ss in by_s.items():
        ds = by_d.get(h)
        if ds and len(ss) == 1 and len(ds) == 1 and len(src[ss[0]][0]) >= 4:
            pair(ss[0], ds[0], 'exact', 1.0)
    print(f'exact: {len(match)}', file=sys.stderr)

    # 1b. exact up to register allocation (unique on both sides)
    sh_s = {a: shape(m) for a, (m, _, _) in src.items() if a not in match}
    sh_d = {a: shape(m) for a, (m, _, _) in dst.items() if a not in used}
    by_s = collections.defaultdict(list)
    by_d = collections.defaultdict(list)
    for a, m in sh_s.items():
        by_s[digest(m)].append(a)
    for a, m in sh_d.items():
        by_d[digest(m)].append(a)
    n0 = len(match)
    for h, ss in by_s.items():
        ds = by_d.get(h)
        if ds and len(ss) == 1 and len(ds) == 1 and len(src[ss[0]][0]) >= 4:
            pair(ss[0], ds[0], 'shape', 1.0)
    print(f'exact up to registers: +{len(match) - n0}', file=sys.stderr)

    # 2. call-graph propagation
    changed = True
    while changed:
        changed = False
        for s, (d, _, _) in list(match.items()):
            cs, cd = src[s][2], dst[d][2]
            if len(cs) != len(cd):
                continue
            for a, b in zip(cs, cd):
                if a in match or b in used or a not in src or b not in dst:
                    continue
                sc = similarity(src[a][0], dst[b][0])
                if sc >= args.min_call_score and pair(a, b, 'calls', sc):
                    changed = True
    print(f'after calls: {len(match)}', file=sys.stderr)

    # 3. order: between matched neighbors
    for _ in range(3):
        anchors = sorted((s, d) for s, (d, _, _) in match.items())
        a_src = [s for s, _ in anchors]
        s_sorted = sorted(src)
        d_sorted = sorted(dst)
        added = 0
        for i in range(len(anchors) - 1):
            (s0, d0), (s1, d1) = anchors[i], anchors[i + 1]
            if d1 <= d0:
                continue
            gs = [a for a in s_sorted[bisect.bisect_right(s_sorted, s0):bisect.bisect_left(s_sorted, s1)]
                  if a not in match]
            gd = [a for a in d_sorted[bisect.bisect_right(d_sorted, d0):bisect.bisect_left(d_sorted, d1)]
                  if a not in used]
            if not gs or not gd or len(gs) * len(gd) > 400:
                continue
            scored = sorted(((similarity(src[a][0], dst[b][0]), a, b) for a in gs for b in gd),
                            reverse=True)
            for sc, a, b in scored:
                if sc >= 0.7 and pair(a, b, 'order', sc):
                    added += 1
        print(f'after order pass: {len(match)} (+{added})', file=sys.stderr)
        if not added:
            break

    # Branch targets: a call or tail call to a matched function must land
    # on its match. Among equally similar candidates (tiny thunks look
    # alike), prefer the one whose targets agree; report agreement for all.
    def targets(funcs, a):
        out = list(funcs[a][2])
        for ins in funcs[a][1]:
            if ins.startswith('b 0x'):
                t = int(ins[2:], 16)
                if not (a <= t < a + 4 * len(funcs[a][1])):
                    out.append(t)
        return out

    def agree(s, d):
        """(hits, known): of s's calls/tail calls to matched functions, how
        many land on their matches among d's (as a multiset: SBX often adds
        calls to out-of-line register save/restore helpers that the other build inlines,
        which shifts positions)."""
        td = collections.Counter(targets(dst, d))
        known = [x for x in targets(src, s) if x in match]
        hits = 0
        for x in known:
            y = match[x][0]
            if td[y]:
                td[y] -= 1
                hits += 1
        return hits, len(known)

    # 3b. gap: exactly one unmatched function on each side between two
    # adjacent matches, doing similar work (same multiset of mnemonics within
    # half): pair them. Catches small library functions whose instructions
    # the compiler scheduled differently.
    def bag_sim(a, b):
        ca = collections.Counter(x.split(' ', 1)[0] for x in a)
        cb = collections.Counter(x.split(' ', 1)[0] for x in b)
        tot = sum((ca | cb).values())
        return sum((ca & cb).values()) / tot if tot else 0.0
    anchors = sorted((s, d) for s, (d, _, _) in match.items())
    s_sorted, d_sorted0 = sorted(src), sorted(dst)
    gap = 0
    for (s0, d0), (s1, d1) in zip(anchors, anchors[1:]):
        if d1 <= d0:
            continue
        gs = [a for a in s_sorted[bisect.bisect_right(s_sorted, s0):bisect.bisect_left(s_sorted, s1)]
              if a not in match]
        gd = [a for a in d_sorted0[bisect.bisect_right(d_sorted0, d0):bisect.bisect_left(d_sorted0, d1)]
              if a not in used]
        if len(gs) == 1 and len(gd) == 1:
            sc = bag_sim(src[gs[0]][0], dst[gd[0]][0])
            if sc >= 0.5 and pair(gs[0], gd[0], 'gap', sc):
                gap += 1
    print(f'gap pairs: +{gap}', file=sys.stderr)

    # Local shift: the median (dst - src) of the exact matches nearest an
    # address. Code moves in blocks, so a match far off its neighbors'
    # shift is suspect.
    exact = sorted((s, d) for s, (d, how, _) in match.items() if how == 'exact')
    ex_src = [s for s, _ in exact]

    def local_shift(a, k=8):
        i = bisect.bisect_left(ex_src, a)
        near = exact[max(0, i - k):i + k]
        ds = sorted(d - s for s, d in near)
        return ds[len(ds) // 2] if ds else 0

    d_sorted = sorted(dst)

    def local_search(s, window=0x800):
        """Best dst within window of the predicted address, any state."""
        p = s + local_shift(s)
        lo, hi = bisect.bisect_left(d_sorted, p - window), bisect.bisect_right(d_sorted, p + window)
        best = (0.0, None)
        for b in d_sorted[lo:hi]:
            sc = max(similarity(src[s][0], dst[b][0]),
                     similarity(shape(src[s][0]), shape(dst[b][0])))
            # Prefer the predicted spot on ties (tiny functions look alike).
            key = (sc, -abs(b - p))
            if best[1] is None or key > (best[0], -abs(best[1] - p)):
                best = (sc, b)
        return best

    # 4. suspicious matches: far from the local shift -> look locally.
    moved = 0
    for s, (d, how, sc) in list(match.items()):
        if how == 'exact':
            continue
        if abs((d - s) - local_shift(s)) <= 0x4000:
            continue
        lsc, b = local_search(s)
        if b is not None and lsc >= max(0.5, sc - 0.05) and (b not in used or b == d) \
                and not (agree(s, b)[1] and agree(s, b)[0] == 0):
            used.discard(d)
            match[s] = (b, 'local', lsc)
            used.add(b)
            moved += 1
    print(f're-placed by locality: {moved}', file=sys.stderr)

    # 5. wanted addresses still unmatched: local search, then global fuzzy.
    wanted = [int(x, 16) for x in args.want.split(',') if x.strip()]
    for s in wanted:
        if s not in src:
            print(f'want {s:08X}: not a function in src', file=sys.stderr)
            continue
        if s in match:
            continue
        lsc, b = local_search(s)
        if b is not None and lsc >= 0.5 and b not in used and not (agree(s, b)[1] and agree(s, b)[0] == 0):
            match[s] = (b, 'local', lsc)
            used.add(b)
            continue
        free = [x for x in dst if x not in used]
        best = max(((similarity(src[s][0], dst[x][0]), x) for x in free
                    if not (agree(s, x)[1] and agree(s, x)[0] == 0)), default=(0, None))
        if best[1] is not None and best[0] >= 0.5:
            match[s] = (best[1], 'fuzzy', best[0])
            used.add(best[1])

    # 6. branch targets (helpers defined above).
    fixed = 0
    for s, (d, how, sc) in list(match.items()):
        ok, n = agree(s, d)
        if n == 0 or ok == n or len(src[s][0]) > 16:
            continue
        p = s + local_shift(s)
        lo, hi = bisect.bisect_left(d_sorted, p - 0x800), bisect.bisect_right(d_sorted, p + 0x800)
        for b in d_sorted[lo:hi]:
            if b == d or (b in used and b != d):
                continue
            if src[s][0] == dst[b][0] or shape(src[s][0]) == shape(dst[b][0]):
                ok2, n2 = agree(s, b)
                if n2 and ok2 == n2:
                    used.discard(d)
                    match[s] = (b, 'target', 1.0)
                    used.add(b)
                    fixed += 1
                    break
    print(f're-placed by branch targets: {fixed}', file=sys.stderr)

    with open(args.out, 'w', newline='\n') as f:
        f.write('src\tdst\tmethod\tscore\tsrc_len\tdst_len\tshift_ok\tcalls_agree\n')
        for s in sorted(match):
            d, how, sc = match[s]
            ok = 'yes' if abs((d - s) - local_shift(s)) <= 0x4000 else 'NO'
            ag, n = agree(s, d)
            f.write(f'{s:08X}\t{d:08X}\t{how}\t{sc:.3f}\t{len(src[s][0])}\t{len(dst[d][0])}\t{ok}\t{ag}/{n}\n')
    print(f'wrote {len(match)} matches to {args.out}', file=sys.stderr)

    for x in [int(x, 16) for x in args.show.split(',') if x.strip()]:
        if x not in match:
            print(f'\n{x:08X}: no match')
            continue
        d, how, sc = match[x]
        print(f'\n=== src {x:08X} -> dst {d:08X} ({how}, {sc:.3f})')
        a, b = src[x][1], dst[d][1]
        for i in range(max(len(a), len(b))):
            l = a[i] if i < len(a) else ''
            r = b[i] if i < len(b) else ''
            flag = ' ' if mask(*(l.split(' ', 1) + [''])[:2]) == mask(*(r.split(' ', 1) + [''])[:2]) else '*'
            print(f'{flag} {l:40} {r}')


if __name__ == '__main__':
    main()
