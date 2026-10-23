"""Field-by-field comparison of two PART files.

    python -m sbx_formats.diff A.part B.part [--all]

Compares the header, each node, each shape record, each stream, each
material record and index buffer, the skin data, the embedded NODE file,
the small sections and the shader section, then the raw bytes. Prints the
first difference with its field name (all of them with --all). Exit code
0 when the files are identical, 1 when they differ.
"""

import argparse
import sys

import numpy as np

from .part_read import read_PART


class Differences(Exception):
    pass


class Collector:
    def __init__(self, stop_at_first):
        self.items = []
        self.stop = stop_at_first

    def add(self, where, detail):
        self.items.append('%s: %s' % (where, detail))
        if self.stop:
            raise Differences()


def _fmt(v):
    if isinstance(v, bytes):
        return v.hex() if len(v) <= 16 else v[:16].hex() + '...'
    if isinstance(v, float):
        return repr(v)
    if isinstance(v, tuple) and len(v) > 8:
        return '(%s, ...)' % ', '.join(_fmt(x) for x in v[:8])
    return str(v)


def _first_byte(a, b):
    """Index of the first differing byte, or None if equal."""
    n = min(len(a), len(b))
    x = np.frombuffer(a[:n], np.uint8) != np.frombuffer(b[:n], np.uint8)
    i = np.flatnonzero(x)
    if len(i):
        return int(i[0])
    return None if len(a) == len(b) else n


def compare_raw(c, where, a, b):
    if a == b:
        return
    i = _first_byte(a, b)
    if len(a) != len(b) and i == min(len(a), len(b)):
        c.add(where, 'size %d vs %d' % (len(a), len(b)))
    else:
        c.add(where, 'byte +0x%X: %s vs %s (sizes %d, %d)'
              % (i, a[i:i + 8].hex(), b[i:i + 8].hex(), len(a), len(b)))


def compare_record(c, where, a, b):
    """Two Records of the same type, field by field."""
    if a.raw == b.raw:
        return
    for name, off, fmt in a.FIELDS:
        va, vb = a.get(name), b.get(name)
        if a.field_bytes(name) != b.field_bytes(name):
            c.add('%s.%s (+0x%X)' % (where, name, off), '%s vs %s' % (_fmt(va), _fmt(vb)))


def compare_stream(c, where, a, b):
    if a is None or b is None:
        if (a is None) != (b is None):
            c.add(where, 'used in one file only')
        return
    for name in ('type', 'stride', 'count', 'size'):
        if getattr(a, name) != getattr(b, name):
            c.add(where + '.' + name, '%s vs %s' % (getattr(a, name), getattr(b, name)))
            return
    if not np.array_equal(a.raw, b.raw):
        rows = np.flatnonzero((a.raw != b.raw).any(1))
        v = int(rows[0])
        col = int(np.flatnonzero(a.raw[v] != b.raw[v])[0])
        c.add(where, 'vertex %d byte %d: %s vs %s (%d vertices differ)'
              % (v, col, a.raw[v].tobytes().hex(), b.raw[v].tobytes().hex(), len(rows)))


def compare_indices(c, where, a, b):
    if a == b:
        return
    ia, ib = np.frombuffer(a, '>u2'), np.frombuffer(b, '>u2')
    n = min(len(ia), len(ib))
    diff = np.flatnonzero(ia[:n] != ib[:n])
    if len(diff):
        k = int(diff[0])
        c.add(where, 'index %d: %d vs %d (counts %d, %d)' % (k, ia[k], ib[k], len(ia), len(ib)))
    else:
        c.add(where, 'count %d vs %d' % (len(ia), len(ib)))


def compare_parts(da, db, stop_at_first=True):
    """Two PART files as bytes -> list of differences (empty if identical)."""
    c = Collector(stop_at_first)
    try:
        _compare(c, da, db)
    except Differences:
        pass
    return c.items


def _region_of(part, offset):
    best = None
    for o, raw, label in part.regions():
        if o <= offset < o + len(raw):
            best = (label, offset - o)
    return best


def _compare(c, da, db):
    a, b = read_PART(da), read_PART(db)
    compare_record(c, 'header', a.header, b.header)
    if len(a.nodes) != len(b.nodes) or len(a.shapes) != len(b.shapes):
        c.add('counts', 'nodes %d/%d, shapes %d/%d'
              % (len(a.nodes), len(b.nodes), len(a.shapes), len(b.shapes)))
    for i, (x, y) in enumerate(zip(a.nodes, b.nodes)):
        compare_record(c, 'node %d (%s)' % (i, x.name), x, y)
    for i, (x, y) in enumerate(zip(a.shapes, b.shapes)):
        tag = 'shape %d (%s)' % (i, x.name)
        compare_record(c, tag, x, y)
        for k in range(3):
            compare_stream(c, '%s stream %d' % (tag, k), x.streams[k], y.streams[k])
        ma, mb = x.material_block, y.material_block
        compare_raw(c, tag + ' material block head', ma.head, mb.head)
        for k, (ra, rb) in enumerate(zip(ma.draws, mb.draws)):
            compare_record(c, '%s material %d' % (tag, k), ra, rb)
            compare_indices(c, '%s indices %d' % (tag, k), ra.indices_raw, rb.indices_raw)
        if len(ma.draws) != len(mb.draws):
            c.add(tag, 'materials %d vs %d' % (len(ma.draws), len(mb.draws)))
        compare_raw(c, tag + ' material block tail', ma.tail, mb.tail)
        compare_raw(c, tag + ' skin positions', x.skin_positions.raw, y.skin_positions.raw)
        compare_raw(c, tag + ' skin normals', x.skin_normals.raw, y.skin_normals.raw)
        compare_raw(c, tag + ' skin', x.skin_raw, y.skin_raw)
    compare_raw(c, 'embedded', a.embedded_raw, b.embedded_raw)
    sa = [r for _, r in a.small_sections]
    sb = [r for _, r in b.small_sections]
    if len(sa) != len(sb):
        c.add('small sections', 'count %d vs %d' % (len(sa), len(sb)))
    for k, (x, y) in enumerate(zip(sa, sb)):
        compare_raw(c, 'small section %d' % k, x, y)
    if a.shader_count != b.shader_count:
        c.add('shaders', 'pair count %d vs %d' % (a.shader_count, b.shader_count))
    compare_raw(c, 'shaders', a.shaders_raw, b.shaders_raw)
    # Anything the model doesn't hold (padding between sections).
    if da != db:
        i = _first_byte(da, db)
        r = _region_of(a, i) if i < len(da) else None
        where = '%s +0x%X' % r if r else 'padding'
        if len(da) != len(db) and i == min(len(da), len(db)):
            c.add('file', 'size %d vs %d' % (len(da), len(db)))
        else:
            c.add('file', 'byte 0x%X (%s): %s vs %s' % (i, where, da[i:i + 8].hex(), db[i:i + 8].hex()))


def main(argv=None):
    ap = argparse.ArgumentParser(description='Compare two PART files field by field.')
    ap.add_argument('a')
    ap.add_argument('b')
    ap.add_argument('--all', action='store_true', help='list every difference, not just the first')
    args = ap.parse_args(argv)
    with open(args.a, 'rb') as f:
        da = f.read()
    with open(args.b, 'rb') as f:
        db = f.read()
    items = compare_parts(da, db, stop_at_first=not args.all)
    if not items:
        print('identical')
        return 0
    for line in items:
        print(line)
    return 1


if __name__ == '__main__':
    sys.exit(main())
