#!/usr/bin/env python3
"""Port addresses from one generated tree to the other (e.g. another build of
the game -> this one),
using the function matches from match_funcs.py.

  - a function start: its match.
  - a code address inside a function: the matched function, at the
    instruction aligned with it (difflib over the masked instructions).
  - a data address: learned from matched functions. Wherever an aligned pair
    of instructions builds a global's address (lis rX,hi then addi/ori/load/
    store off(rX)), the two addresses are recorded; the most-voted partner
    wins. An address never seen exactly takes the delta of the nearest seen
    address within 0x200 below it (same object, assumed same layout).

  port_addrs.py <src gen dir> <dst gen dir> <matches.tsv> <addr> [addr ...]
                [--from-file doc.md] [--tsv out.tsv]

--from-file takes every sub_XXXXXXXX and 0x8XXXXXXX in a document (e.g.
the other build's docs/functions.md).
"""
import argparse
import bisect
import collections
import difflib
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import match_funcs  # noqa: E402

MEM_RE = re.compile(r'^([rfv]\d+),(-?\d+)\((r\d+)\)$')
STORE_PREFIX = ('st',)


def globals_in(raw):
    """[(index, address)] for instructions that form lis-based addresses."""
    hi = {}
    out = []
    for i, ins in enumerate(raw):
        mn, _, ops = ins.partition(' ')
        parts = ops.split(',')
        if mn == 'lis' and len(parts) == 2:
            hi[parts[0]] = (int(parts[1]) << 16) & 0xFFFFFFFF
            continue
        if mn in ('addi', 'ori') and len(parts) == 3 and parts[1] in hi:
            try:
                off = int(parts[2])
            except ValueError:
                off = None
            if off is not None:
                out.append((i, (hi[parts[1]] + off) & 0xFFFFFFFF))
        m = MEM_RE.match(ops)
        if m and m.group(3) in hi:
            out.append((i, (hi[m.group(3)] + int(m.group(2))) & 0xFFFFFFFF))
        # A write to a register ends what we know about it (stores write
        # memory, not their first operand).
        if parts and parts[0] in hi and not mn.startswith(STORE_PREFIX) and mn != 'lis':
            if not mn.startswith(('cmp', 'b', 'tw', 'dcb', 'mt')):
                hi.pop(parts[0], None)
    return out


def align(a, b):
    """{index in a: index in b} for equal blocks of masked instructions."""
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    m = {}
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag in ('equal', 'replace') and (i2 - i1) == (j2 - j1):
            for k in range(i2 - i1):
                m[i1 + k] = j1 + k
    return m


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('src')
    ap.add_argument('dst')
    ap.add_argument('matches')
    ap.add_argument('addrs', nargs='*')
    ap.add_argument('--from-file')
    ap.add_argument('--tsv')
    args = ap.parse_args()

    src, dst = match_funcs.load(args.src), match_funcs.load(args.dst)
    match = {}
    with open(args.matches) as f:
        next(f)
        for line in f:
            s, d, how, sc, *rest = line.rstrip('\n').split('\t')
            ok = rest[2] if len(rest) > 2 else 'yes'
            ag = rest[3] if len(rest) > 3 else ''
            tag = how + ('' if ok == 'yes' else ' FAR') + (f' agree {ag}' if ag and ag.split('/')[1] != '0' else '')
            match[int(s, 16)] = (int(d, 16), tag, float(sc))
    s_starts = sorted(src)

    # Data map from matched functions.
    votes = collections.defaultdict(collections.Counter)
    for s, (d, how, sc) in match.items():
        if sc < 0.9 or s not in src or d not in dst:
            continue
        gs, gd = dict(globals_in(src[s][1])), dict(globals_in(dst[d][1]))
        if not gs:
            continue
        al = align(src[s][0], dst[d][0])
        for i, a in gs.items():
            j = al.get(i)
            if j is not None and j in gd:
                votes[a][gd[j]] += 1
    data = {a: c.most_common(1)[0] for a, c in votes.items()}
    data_keys = sorted(data)
    print(f'data addresses learned: {len(data)}', file=sys.stderr)

    wanted = []
    if args.from_file:
        text = open(args.from_file, encoding='utf-8').read()
        wanted += [int(x, 16) for x in re.findall(r'sub_([0-9A-Fa-f]{8})', text)]
        wanted += [int(x, 16) for x in re.findall(r'\b0x(8[0-9A-Fa-f]{7})\b', text)]
    wanted += [int(a.replace('sub_', '0x'), 16) for a in args.addrs]
    seen = set()
    rows = []
    for a in wanted:
        if a in seen:
            continue
        seen.add(a)
        if a in src:
            if a in match:
                d, how, sc = match[a]
                rows.append((a, d, 'function', f'{how} {sc:.3f}'))
            else:
                rows.append((a, None, 'function', 'unmatched'))
            continue
        i = bisect.bisect_right(s_starts, a) - 1
        owner = s_starts[i] if i >= 0 else None
        if owner is not None and (a - owner) // 4 < len(src[owner][1]) and a < 0x825C2000:
            k = (a - owner) // 4
            if owner in match:
                d, how, sc = match[owner]
                j = align(src[owner][0], dst[d][0]).get(k)
                if j is not None:
                    rows.append((a, d + 4 * j, 'code', f'in sub_{owner:08X}+{4*k:X} -> sub_{d:08X}+{4*j:X} ({how} {sc:.3f})'))
                else:
                    rows.append((a, None, 'code', f'in sub_{owner:08X}+{4*k:X}: no aligned instruction'))
            else:
                rows.append((a, None, 'code', f'in unmatched sub_{owner:08X}'))
            continue
        if a in data:
            b, n = data[a]
            rows.append((a, b, 'data', f'{n} refs'))
            continue
        j = bisect.bisect_right(data_keys, a) - 1
        if j >= 0 and a - data_keys[j] <= 0x200:
            base = data_keys[j]
            b, n = data[base]
            rows.append((a, (b + a - base) & 0xFFFFFFFF, 'data',
                         f'via 0x{base:08X}+{a-base:X} ({n} refs)'))
        else:
            rows.append((a, None, 'data', 'not referenced by matched code'))

    out = open(args.tsv, 'w', newline='\n') if args.tsv else None
    if out:
        out.write('src\tdst\tkind\thow\n')
    for a, b, kind, how in rows:
        bs = f'{b:08X}' if b is not None else '-'
        print(f'{a:08X}  {bs:8}  {kind:8}  {how}')
        if out:
            out.write(f'{a:08X}\t{bs}\t{kind}\t{how}\n')


if __name__ == '__main__':
    main()
