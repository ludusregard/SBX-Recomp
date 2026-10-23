#!/usr/bin/env python3
"""Reader coverage report for PART files.

    python tests/coverage.py BUILD_OUT [-o REPORT.md]

Reads every .part file with sbx_formats and decodes it (decode.py),
and reports what it saw: stream types per slot, primitive types, shader
pair counts, material fields, sections, and the values of the fields not
worked out. Exit code 1 if any file fails to read.
"""

import argparse
import collections
import glob
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

from sbx_formats import layout  # noqa: E402
from sbx_formats.decode import decode_PART  # noqa: E402
from sbx_formats.part_read import read_PART  # noqa: E402


class Tally:
    """Counts with the first few example files for each key."""

    def __init__(self):
        self.count = collections.Counter()
        self.files = collections.defaultdict(set)
        self.examples = collections.defaultdict(list)

    def add(self, key, name, n=1):
        self.count[key] += n
        self.files[key].add(name)
        if len(self.examples[key]) < 3 and name not in self.examples[key]:
            self.examples[key].append(name)

    def rows(self, key_fmt=str):
        for k, v in sorted(self.count.items(), key=lambda kv: (-kv[1], str(kv[0]))):
            yield key_fmt(k), v, len(self.files[k]), ', '.join(self.examples[k])


def table(title, header, rows):
    out = ['### ' + title, '', '| ' + ' | '.join(header) + ' |',
           '|' + '---|' * len(header)]
    out += ['| ' + ' | '.join(str(c) for c in r) + ' |' for r in rows]
    return out + ['']


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('folder')
    ap.add_argument('-o', '--output', help='write the report here (Markdown)')
    args = ap.parse_args()

    files = sorted(glob.glob(os.path.join(args.folder, '*.part')))
    empty = [f for f in files if os.path.getsize(f) == 0]
    files = [f for f in files if os.path.getsize(f) > 0]

    errors, wrapper_errors = [], []
    t = collections.defaultdict(Tally)
    totals = collections.Counter()
    unknown_values = collections.defaultdict(collections.Counter)
    unknown_shapes = []

    for f in files:
        name = os.path.basename(f)
        with open(f, 'rb') as fh:
            d = fh.read()
        try:
            p = read_PART(d)
        except Exception as e:
            errors.append('%s: %s' % (name, e))
            continue
        try:
            decode_PART(p)
        except Exception as e:
            wrapper_errors.append('%s: %s' % (name, e))

        h = p.header
        totals['files'] += 1
        totals['bytes'] += len(d)
        totals['nodes'] += len(p.nodes)
        totals['markers'] += sum(1 for n in p.nodes if not n.is_shape_node)
        totals['shapes'] += len(p.shapes)
        if not p.shapes:
            totals['files without shapes'] += 1
        t['shader pairs'].add(p.shader_count, name)
        t['embedded'].add(p.embedded_raw[:4].hex() if p.embedded_raw else 'none', name)
        t['small sections'].add(len(p.small_sections), name)
        t['section order'].add(' > '.join(_order(p)), name)
        for fname, _, fmt in layout.HEADER_FIELDS:
            if fname.startswith('unknown_'):
                unknown_values['header.' + fname][_val(h, fname)] += 1
        unknown_values['header.nodes_offset_2 == nodes_offset'][h.get('nodes_offset_2') == h.get('nodes_offset')] += 1

        for s in p.shapes:
            for k, st in enumerate(s.streams):
                if st is not None:
                    t['streams'].add((k, st.type, st.stride, st.known), name)
                    if st.size != st.count * st.stride:
                        t['stream size != count x stride'].add((k, st.type), name)
            t['stream layouts'].add(tuple((st.type, st.stride) for st in s.streams if st is not None), name)
            for which, a in (('positions', s.skin_positions), ('normals', s.skin_normals)):
                key = (which, a.stream.type, a.stream.stride) if a.stream else (which, 'empty')
                t['skin arrays'].add(key, name)
            skin_size = int.from_bytes(s.skin_raw[4:8], 'big')
            t['skin blocks'].add('static (0x20)' if skin_size == 0x20 else 'skinned', name)
            t['shape matrix'].add('identity' if np.allclose(s.matrix, np.eye(4)) else 'other', name)
            if s.unknown_streams():
                unknown_shapes.append((name, s.name, [(st.type, st.stride, st.count) for st in s.unknown_streams()]))
            for fname, _, fmt in layout.SHAPE_FIELDS:
                if fname.startswith('unknown_'):
                    unknown_values['shape.' + fname][_val(s, fname)] += 1
            mb = s.material_block
            t['material block tail'].add(mb.tail.hex(), name)
            for dr in mb.draws:
                totals['draws'] += 1
                totals['indices'] += dr.get('index_count')
                t['primitive type'].add(dr.get('primitive_type'), name)
                t['texture count'].add(dr.get('texture_count'), name)
                t['shared shader'].add(dr.get('shared_shader'), name)
                t['own shader index < pair count'].add(dr.get('shader') < p.shader_count, name)
                t['blend'].add(dr.get('blend'), name)
                t['flags'].add(dr.get('flags'), name)
                for fname, _, fmt in layout.MATERIAL_FIELDS:
                    if fname.startswith('unknown_'):
                        unknown_values['material.' + fname][_val(dr, fname)] += 1

    out = ['# PART reader coverage', '',
           'Folder: `%s`. %d part files (%d empty entries skipped), %.1f MB.'
           % (os.path.basename(os.path.normpath(args.folder)), len(files), len(empty), totals['bytes'] / 1e6), '']
    out += ['- sbx_formats read errors: **%d**' % len(errors)]
    out += ['  - ' + e for e in errors[:20]]
    out += ['- decode errors: **%d**' % len(wrapper_errors)]
    out += ['  - ' + e for e in wrapper_errors[:20]]
    out += ['- %d nodes (%d markers), %d shapes, %d draws, %d indices; %d files have no shapes'
            % (totals['nodes'], totals['markers'], totals['shapes'], totals['draws'], totals['indices'],
               totals['files without shapes']), '']

    def rows3(name, fmt=str):
        return [(a, b, c, d) for a, b, c, d in t[name].rows(fmt)]

    out += table('Streams (slot, type, stride)', ['Slot, type, stride', 'Known', 'Shapes', 'Files', 'Examples'],
                 [('%d, 0x%02X, %d' % k[:3], 'yes' if k[3] else '**no, raw**', n, nf, ex)
                  for k, n, nf, ex in _raw_rows(t['streams'])])
    out += table('Stream layouts per shape', ['Layout (type, stride)', 'Shapes', 'Files', 'Examples'],
                 rows3('stream layouts', lambda k: ' '.join('0x%02X/%d' % x for x in k)))
    for title in ('skin arrays', 'skin blocks', 'shape matrix', 'primitive type', 'shader pairs',
                  'shared shader', 'own shader index < pair count', 'texture count', 'blend', 'flags',
                  'material block tail', 'embedded', 'small sections', 'section order',
                  'stream size != count x stride'):
        if t[title].count:
            out += table(title.capitalize(), ['Value', 'Count', 'Files', 'Examples'], rows3(title))

    out += ['### Shapes with unknown stream types', '',
            '%d shapes in %d files. First 20:' % (len(unknown_shapes), len({u[0] for u in unknown_shapes})), '']
    out += ['- %s `%s`: %s' % (f, s, ', '.join('type 0x%02X stride %d, %d vertices' % x for x in st))
            for f, s, st in unknown_shapes[:20]]
    out += ['']

    out += ['### Fields not worked out: distinct values', '',
            '| Field | Distinct values | Most common (count) |', '|---|---|---|']
    for k in sorted(unknown_values):
        c = unknown_values[k]
        top = ', '.join('%s (%d)' % (v, n) for v, n in c.most_common(3))
        out.append('| %s | %d | %s |' % (k, len(c), top))
    out.append('')

    text = '\n'.join(out)
    if args.output:
        with open(args.output, 'w') as fh:
            fh.write(text)
    print(text)
    return 1 if errors or wrapper_errors else 0


def _raw_rows(tally):
    for k, v in sorted(tally.count.items(), key=lambda kv: kv[0][:3]):
        yield k, v, len(tally.files[k]), ', '.join(tally.examples[k])


def _val(rec, fname):
    v = rec.get(fname)
    if isinstance(v, bytes):
        return 'zero' if not any(v) else v.hex()[:24] + ('...' if len(v) > 12 else '')
    if isinstance(v, tuple):
        return str(v)
    return '0x%X' % v if isinstance(v, int) else v


def _order(p):
    kinds = []
    for off, raw, label in sorted(p.regions(), key=lambda r: r[0]):
        k = label.split(' (')[0] if not label.startswith('shape') else label.split(') ')[-1] if ') ' in label else 'shape'
        k = k.rstrip('0123456789 ').replace('material block head', 'materials').replace('material block tail', 'materials')
        k = {'material': 'materials', 'node': 'nodes', 'shape': 'shapes', 'small section': 'small sections',
             'skin positions': 'skin arrays', 'skin normals': 'skin arrays', 'stream': 'streams'}.get(k, k)
        if not kinds or kinds[-1] != k:
            kinds.append(k)
    return kinds


if __name__ == '__main__':
    sys.exit(main())
