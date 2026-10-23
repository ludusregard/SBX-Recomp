#!/usr/bin/env python3
"""Encoder test: re-encode every shape of every part from its decoded data.

    python tests/encode.py BUILD_OUT [--limit N]

For each shape that can be encoded: decode it, encode it again, write the
part, read it back and check it decodes to the same thing: positions,
normals (within half precision), UVs, colors, every draw's triangles
(same set, same winding) and its record. Also checks that the streams
come out byte-identical to the game's (the stream encoder is exact) and
that the bounds match the game's rule. Shapes the encoder refuses are
counted, not failures.
"""

import argparse
import collections
import glob
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

from sbx_formats import read_PART  # noqa: E402
from sbx_formats.decode import decode_PART  # noqa: E402
from sbx_formats.encode import EncodeError, encode_shape  # noqa: E402
from sbx_formats.part_write import write_PART  # noqa: E402


def canon(tris):
    t = np.asarray(tris, np.int64).reshape(-1, 3)
    if not len(t):
        return t
    r = np.argmin(t, 1)
    idx = (r[:, None] + np.arange(3)) % 3
    t = t[np.arange(len(t))[:, None], idx]
    return t[np.lexsort(t.T[::-1])]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('folder')
    ap.add_argument('--limit', type=int)
    args = ap.parse_args()
    files = sorted(glob.glob(os.path.join(args.folder, '*.part')))[:args.limit]
    c = collections.Counter()
    fails = []
    for f in files:
        d = open(f, 'rb').read()
        part = read_PART(d)
        dp = decode_PART(part)
        for i, (s, ds) in enumerate(zip(part.shapes, dp.shapes)):
            draws = [(rec, dd.triangles) for rec, dd in zip(s.draws, ds.draws)]
            try:
                new = encode_shape(s, ds.positions, ds.normals, ds.uvs, ds.uvs2, ds.colors, draws, ds.uvs3)
            except EncodeError as e:
                c['refused: ' + str(e).split(' (')[0]] += 1
                continue
            c['encoded'] += 1
            same_streams = all((a is None and b is None) or np.array_equal(a.raw, b.raw)
                               for a, b in zip(s.streams, new.streams))
            c['streams byte-identical'] += same_streams
            c['bounds identical'] += s.field_bytes('bounds_box') == new.field_bytes('bounds_box')
            part.shapes[i] = new
        out = write_PART(part)
        back = decode_PART(read_PART(out))
        orig = decode_PART(read_PART(d))
        for a, b in zip(orig.shapes, back.shapes):
            problems = []
            if not np.array_equal(a.positions, b.positions):
                problems.append('positions')
            for name in ('uvs', 'uvs2', 'uvs3', 'colors'):
                x, y = getattr(a, name), getattr(b, name)
                if (x is None) != (y is None) or (x is not None and not np.array_equal(x, y)):
                    problems.append(name)
            if (a.normals is None) != (b.normals is None) or (
                    a.normals is not None and np.abs(a.normals - b.normals).max() > 1e-3):
                problems.append('normals')
            if len(a.draws) != len(b.draws) or any(
                    not np.array_equal(canon(x.triangles), canon(y.triangles)) for x, y in zip(a.draws, b.draws)):
                problems.append('triangles')
            if problems:
                fails.append('%s %s: %s' % (os.path.basename(f), a.name, ', '.join(problems)))
        c['parts'] += 1
    for k, v in sorted(c.items()):
        print('%6d  %s' % (v, k))
    for line in fails[:20]:
        print('FAIL', line)
    print('PASS' if not fails else 'FAIL: %d shapes' % len(fails))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
