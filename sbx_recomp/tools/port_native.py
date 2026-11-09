#!/usr/bin/env python3
"""Port another build's native C++ (e.g. ITC-Recomp's itc_recomp/src/) to this
build's addresses, and grade how safe each file is to bring over.

Every guest address in the source is found and mapped with the function
matches (match_funcs.py) and the data map (port_addrs.py's, learned from
aligned global references in matched functions):

  sub_XXXXXXXX, __imp__sub_XXXXXXXX   a function (or a code address)
  0x8XXXXXXX                          a function, code or data address
  bare XXXXXXXX as a macro argument  a function, as in FALLBACK(820CC248)
  NameXXXXXXXX                        a table or function named after one
                                      (kXXXXXXXX, CameraModesStepXXXXXXXX)

Grades, per address:
  A  exact: function matched exact/shape/pinned (same instructions with
     addresses masked, so the same struct offsets and constants); data seen
     at 3 or more aligned references.
  B  high: a function matched at 0.95 or more, or with every call agreeing
     at 0.8 or more; data seen at 1-2 references.
  C  check: anything else matched (a lower score, data only by the nearest
     referenced address); confirm in the generated code.
  D  none: no match. The file can't be ported as is.

A native function's own grade matters most: its C++ re-implements the
original's struct offsets and constants, which only an exact (A) match
guarantees are the same here. Those are the functions named in
ITC_NATIVE_RUNNING("sub_XXXXXXXX ...") lines; they're counted separately.
The harnesses (src/harnesses/), which compare the native code with the
recompiled original, are the check for everything below A.

  port_native.py <src gen dir> <dst gen dir> <matches.tsv> <src tree>
                 [--out DIR] [--report report.tsv] [--summary summary.md]
                 [--only subsystem,...]

--out writes the tree with every graded A/B/C address rewritten (D left as is
and marked with a "// PORT: unmapped" comment on its line). Comments and
strings are rewritten too, so the names in logs and flags stay right.
"""
import argparse
import bisect
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import match_funcs  # noqa: E402
import port_addrs  # noqa: E402

TOKEN_RE = re.compile(
    r'(?P<sub>(?:__imp__)?sub_(?P<subaddr>8[0-9A-Fa-f]{7}))'
    r'|(?P<hex>0x(?P<hexaddr>8[0-9A-Fa-f]{7}))\b'
    r'|(?:(?<=\()|(?<=, )|(?<=,))(?P<bare>8[0-9A-F]{7})(?=\s*[,)])'
    r'|\b(?P<kpre>[A-Za-z_](?:[A-Za-z0-9_]*?[A-Za-z_])?)(?P<kaddr>8[0-9A-F]{7})\b')
NATIVE_RE = re.compile(r'ITC_NATIVE_RUNNING\("sub_(8[0-9A-Fa-f]{7})')
GRADE_ORDER = 'ABCD'
SOURCE_EXT = ('.cpp', '.h', '.inc', '.hpp')


class Mapper:
    def __init__(self, src_dir, dst_dir, matches_path):
        self.src = match_funcs.load(src_dir)
        self.dst = match_funcs.load(dst_dir)
        self.starts = sorted(self.src)
        self.match = {}
        with open(matches_path) as f:
            next(f)
            for line in f:
                s, d, how, sc, *rest = line.rstrip('\n').split('\t')
                agree = rest[3] if len(rest) > 3 else ''
                self.match[int(s, 16)] = (int(d, 16), how, float(sc), agree)
        votes = collections.defaultdict(collections.Counter)
        for s, (d, how, sc, _) in self.match.items():
            if sc < 0.9 or s not in self.src or d not in self.dst:
                continue
            gs = dict(port_addrs.globals_in(self.src[s][1]))
            gd = dict(port_addrs.globals_in(self.dst[d][1]))
            if not gs:
                continue
            al = port_addrs.align(self.src[s][0], self.dst[d][0])
            for i, a in gs.items():
                j = al.get(i)
                if j is not None and j in gd:
                    votes[a][gd[j]] += 1
        self.data = {a: c.most_common(1)[0] for a, c in votes.items()}
        self.data_keys = sorted(self.data)
        self.cache = {}
        self.align_cache = {}

    def function_grade(self, how, sc, agree):
        if how in ('exact', 'shape', 'pinned'):
            return 'A'
        full = False
        if agree and '/' in agree:
            n, t = agree.split('/')
            full = t != '0' and n == t
        if sc >= 0.95 or (full and sc >= 0.8):
            return 'B'
        return 'C'

    def map(self, a):
        """(dst address or None, grade, kind, note)"""
        if a in self.cache:
            return self.cache[a]
        r = self._map(a)
        self.cache[a] = r
        return r

    def _map(self, a):
        if a in self.src:
            if a in self.match:
                d, how, sc, agree = self.match[a]
                return d, self.function_grade(how, sc, agree), 'function', f'{how} {sc:.3f}'
            return None, 'D', 'function', 'unmatched'
        i = bisect.bisect_right(self.starts, a) - 1
        owner = self.starts[i] if i >= 0 else None
        if owner is not None and (a - owner) // 4 < len(self.src[owner][1]) and a % 4 == 0:
            k = (a - owner) // 4
            if owner not in self.match:
                return None, 'D', 'code', f'in unmatched sub_{owner:08X}'
            d, how, sc, agree = self.match[owner]
            key = (owner, d)
            if key not in self.align_cache:
                self.align_cache[key] = port_addrs.align(self.src[owner][0], self.dst[d][0])
            j = self.align_cache[key].get(k)
            if j is None:
                return None, 'D', 'code', f'in sub_{owner:08X}+{4*k:X}: no aligned instruction'
            return d + 4 * j, self.function_grade(how, sc, agree), 'code', \
                f'sub_{owner:08X}+{4*k:X} ({how} {sc:.3f})'
        if a in self.data:
            b, n = self.data[a]
            return b, 'A' if n >= 3 else 'B', 'data', f'{n} refs'
        j = bisect.bisect_right(self.data_keys, a) - 1
        if j >= 0 and a - self.data_keys[j] <= 0x200:
            base = self.data_keys[j]
            b, n = self.data[base]
            return (b + a - base) & 0xFFFFFFFF, 'C', 'data', f'via 0x{base:08X}+{a-base:X}'
        return None, 'D', 'data', 'not referenced by matched code'


def subsystem(rel):
    parts = rel.replace('\\', '/').split('/')
    return parts[0] if len(parts) > 1 else '(top)'


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('src_gen')
    ap.add_argument('dst_gen')
    ap.add_argument('matches')
    ap.add_argument('tree')
    ap.add_argument('--out')
    ap.add_argument('--report')
    ap.add_argument('--summary')
    ap.add_argument('--only')
    args = ap.parse_args()

    m = Mapper(args.src_gen, args.dst_gen, args.matches)
    print(f'functions {len(m.src)} -> {len(m.dst)}, matches {len(m.match)}, '
          f'data addresses learned {len(m.data)}', file=sys.stderr)
    only = set(args.only.split(',')) if args.only else None

    rows = []          # (file, line, address, dst, grade, kind, note)
    natives = collections.defaultdict(list)  # subsystem -> [(addr, grade, note)]
    files = collections.defaultdict(lambda: collections.Counter())
    for root, _, names in os.walk(args.tree):
        for name in sorted(names):
            if not name.endswith(SOURCE_EXT):
                continue
            path = os.path.join(root, name)
            rel = os.path.relpath(path, args.tree).replace('\\', '/')
            sub = subsystem(rel)
            if only and sub not in only:
                continue
            text = open(path, encoding='utf-8', errors='replace').read()
            out_lines = []
            for n, line in enumerate(text.split('\n'), 1):
                unmapped = False

                def repl(mt):
                    nonlocal unmapped
                    if mt.group('sub'):
                        a = int(mt.group('subaddr'), 16)
                    elif mt.group('hex'):
                        a = int(mt.group('hexaddr'), 16)
                    elif mt.group('kaddr'):
                        a = int(mt.group('kaddr'), 16)
                        if a not in m.src:
                            return mt.group(0)
                    else:
                        a = int(mt.group('bare'), 16)
                        if a not in m.src:
                            return mt.group(0)  # not a function: leave it
                    d, grade, kind, note = m.map(a)
                    rows.append((rel, n, a, d, grade, kind, note))
                    files[rel][grade] += 1
                    if d is None:
                        unmapped = True
                        return mt.group(0)
                    tok = mt.group(0)
                    if mt.group('sub'):
                        prefix = tok[:tok.index('sub_')]
                        return f'{prefix}sub_{d:08X}'
                    if mt.group('kaddr'):
                        return f"{mt.group('kpre')}{d:08X}"
                    if mt.group('hex'):
                        lower = mt.group('hexaddr').islower()
                        s = f'{d:08x}' if lower else f'{d:08X}'
                        return f'0x{s}'
                    return f'{d:08X}'

                new = TOKEN_RE.sub(repl, line)
                for nm in NATIVE_RE.finditer(line):
                    a = int(nm.group(1), 16)
                    d, grade, kind, note = m.map(a)
                    natives[sub].append((a, grade, note, rel))
                if unmapped:
                    new += '  // PORT: unmapped address on this line'
                out_lines.append(new)
            if args.out:
                dst = os.path.join(args.out, rel)
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                with open(dst, 'w', encoding='utf-8', newline='') as f:
                    f.write('\n'.join(out_lines))

    if args.report:
        with open(args.report, 'w', newline='\n') as f:
            f.write('file\tline\tsrc\tdst\tgrade\tkind\thow\n')
            for rel, n, a, d, g, k, note in rows:
                ds = f'{d:08X}' if d is not None else '-'
                f.write(f'{rel}\t{n}\t{a:08X}\t{ds}\t{g}\t{k}\t{note}\n')

    # Per subsystem: distinct addresses by grade, native functions by grade.
    by_sub = collections.defaultdict(dict)
    for rel, n, a, d, g, k, note in rows:
        by_sub[subsystem(rel)][a] = g
    lines = ['| Subsystem | Addresses A/B/C/D | Native functions A/B/C/D | Ready |',
             '|---|---|---|---|']
    totals = collections.Counter()
    ntotals = collections.Counter()
    for sub in sorted(set(by_sub) | set(natives)):
        c = collections.Counter(by_sub.get(sub, {}).values())
        nat = {}
        for a, g, note, rel in natives.get(sub, []):
            nat[a] = g
        nc = collections.Counter(nat.values())
        totals.update(c)
        ntotals.update(nc)
        if c['D'] == 0 and nc['C'] == 0 and nc['B'] == 0:
            ready = 'yes'
        elif c['D'] == 0:
            ready = 'harness'
        else:
            ready = 'no'
        lines.append(f'| {sub} | {c["A"]}/{c["B"]}/{c["C"]}/{c["D"]} | '
                     f'{nc["A"]}/{nc["B"]}/{nc["C"]}/{nc["D"]} | {ready} |')
    lines.append(f'| **all** | {totals["A"]}/{totals["B"]}/{totals["C"]}/{totals["D"]} | '
                 f'{ntotals["A"]}/{ntotals["B"]}/{ntotals["C"]}/{ntotals["D"]} | |')
    text = '\n'.join(lines) + '\n'
    if args.summary:
        open(args.summary, 'w', newline='\n').write(text)
    print(text)


if __name__ == '__main__':
    main()
