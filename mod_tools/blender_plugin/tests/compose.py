#!/usr/bin/env python3
"""Composition test: add shapes, markers and shader pairs to every part.

    python tests/compose.py BUILD_OUT [--limit N]

For every part with shapes, without Blender:
- the game's part follows the rules compose.check_rules checks;
- two new shapes are added, copies of the part's first shape that can
  lend its layout, named to sort first and last ('aaa_new_Shape',
  'zzz_new_Shape'); a marker ('zzz_new_ef') at (1, 2, 3); and a shader
  pair from another part, which a copy of the first draw then uses;
- the result writes, reads back and still follows the rules, the new
  shapes decode to their copies' geometry, the marker sits where it was
  put, the old shapes decode as before and the old records only moved;
- the part without additions still writes byte for byte.
"""

import argparse
import collections
import glob
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

from sbx_formats import read_PART, shaders  # noqa: E402
from sbx_formats.compose import (ComposeError, add_marker, add_shader_pair, add_shape,  # noqa: E402
                                 check_rules)
from sbx_formats.decode import decode_PART  # noqa: E402
from sbx_formats.encode import EncodeError, encode_new_shape, template_problem  # noqa: E402
from sbx_formats.part_model import Draw  # noqa: E402
from sbx_formats.part_write import write_PART  # noqa: E402

from encode import canon  # noqa: E402

# Record bytes the writer sets from the layout; the rest must not change.
PLACEMENT = ('index_count', 'primitive_count', 'index_offset', 'index_size', 'index_file_offset')


def record_body(rec):
    b = bytearray(rec.raw)
    for name in PLACEMENT:
        _, off, _ = Draw.field(name)
        b[off:off + 4] = b'\0\0\0\0'
    return bytes(b)


def same_geometry(a, b):
    if len(a.positions) != len(b.positions) or not np.allclose(a.positions, b.positions, atol=1e-6):
        return 'positions'
    if len(a.draws) != len(b.draws):
        return 'draw count'
    for x, y in zip(a.draws, b.draws):
        if not np.array_equal(canon(x.triangles), canon(y.triangles)):
            return 'triangles'
    for name in ('uvs', 'uvs2', 'colors'):
        u, v = getattr(a, name), getattr(b, name)
        if (u is None) != (v is None) or (u is not None and not np.allclose(u, v, atol=1e-3)):
            return name
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('folder')
    ap.add_argument('--limit', type=int)
    args = ap.parse_args()
    files = sorted(glob.glob(os.path.join(args.folder, '*.part')))[:args.limit]
    # A shader pair to bring into each part: the first one seen that the part lacks.
    library = {}
    c = collections.Counter()
    fails = []
    for f in files:
        d = open(f, 'rb').read()
        if not d:
            continue
        part = read_PART(d)
        _, pairs, _ = shaders.split_section(part.shaders_raw)
        for p in pairs:
            library.setdefault(p.vs_hash + p.ps_hash, p)
    for f in files:
        name = os.path.basename(f)
        d = open(f, 'rb').read()
        if not d:
            continue
        part = read_PART(d)
        if write_PART(part) != d:
            fails.append('%s: doesn\'t write back' % name)
            continue
        problems = check_rules(part)
        if problems:
            c['game part breaks the rules (left out): %s' % problems[0]] += 1
            continue
        if not part.shapes or part.embedded_nodes is None:
            c['no shapes (left out)'] += 1
            continue
        src = next((s for s in part.shapes if template_problem(s) is None), None)
        if src is None:
            c['no shape that can lend its layout (left out)'] += 1
            continue
        before = decode_PART(part)
        k = part.shapes.index(src)
        dsrc = before.shapes[k]
        old_records = {(s.name, j): record_body(r) for s in part.shapes for j, r in enumerate(s.draws)}
        _, pairs, _ = shaders.split_section(part.shaders_raw)
        have = {p.vs_hash + p.ps_hash for p in pairs}
        foreign = next((p for key, p in library.items() if key not in have), None)
        try:
            new_slot = add_shader_pair(part, foreign) if foreign is not None else None
            draws = [(r, dd.triangles) for r, dd in zip(src.draws, dsrc.draws)]
            if new_slot is not None:
                r = Draw(src.draws[0].raw)
                r.set('shader', new_slot)
                draws[0] = (r, draws[0][1])
            for nm in ('aaa_new_Shape', 'zzz_new_Shape'):
                raw_name = nm.encode() + b'\0' * (32 - len(nm))
                shape = encode_new_shape(src, raw_name, dsrc.positions, dsrc.normals, dsrc.uvs, dsrc.uvs2,
                                         dsrc.colors, draws, dsrc.uvs3)
                add_shape(part, shape)
            add_marker(part, 'zzz_new_ef', (1.0, 2.0, 3.0))
        except (ComposeError, EncodeError) as e:
            fails.append('%s: %s' % (name, e))
            continue
        data = write_PART(part)
        back = read_PART(data)
        problems = check_rules(back)
        if problems:
            fails.append('%s: after adding: %s' % (name, '; '.join(problems[:3])))
            continue
        after = decode_PART(back)
        by_name = {s.name: s for s in after.shapes}
        why = None
        for s in before.shapes:
            w = same_geometry(s, by_name.get(s.name))
            if w:
                why = '%s changed (%s)' % (s.name, w)
        for nm in ('aaa_new_Shape', 'zzz_new_Shape'):
            w = same_geometry(dsrc, by_name[nm])
            if w:
                why = '%s isn\'t its copy (%s)' % (nm, w)
        for s in back.shapes:
            for j, r in enumerate(s.draws):
                if (s.name, j) in old_records and record_body(r) != old_records[(s.name, j)]:
                    why = '%s draw %d record changed' % (s.name, j)
        mk = next((m for m in after.markers if m.name == 'zzz_new_ef'), None)
        if mk is None or not np.allclose(mk.position, (1, 2, 3), atol=1e-6):
            why = 'marker not where it was put'
        if new_slot is not None:
            first = by_name['aaa_new_Shape'].draws[0].shader
            if first != new_slot:
                why = 'new draw uses shader slot %d, not the new pair %d' % (first, new_slot)
        if why:
            fails.append('%s: %s' % (name, why))
            continue
        c['parts with additions, read back right'] += 1
        c['... with a shader pair brought in'] += new_slot is not None
    for k, v in sorted(c.items()):
        print('%6d  %s' % (v, k))
    for f in fails[:20]:
        print('FAIL', f)
    print('PASS' if not fails else '%d FAILED' % len(fails))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
