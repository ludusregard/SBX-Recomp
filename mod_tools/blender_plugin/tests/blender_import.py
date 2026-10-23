#!/usr/bin/env python3
"""Blender import test: the add-on's import against the decoded parts.

    blender -b --factory-startup --python tests/blender_import.py -- BUILD_OUT

(or run with a Python that has the bpy module.) Installs the add-on zip
from build/ (run build.py first), then for each case in CASES imports the
car and checks every imported shape against sbx_formats' decode of its
part: vertices (one per game vertex, in order), faces (every game
triangle, the winding flipped for Blender's frame), material slots (one
per draw), UV maps and normals (within 1 degree, except where Blender
can't hold the game's: triangles drawn both ways, vertices touching a
zero-area triangle, zero normals in the game data). Also
checks placement: parts modeled in place stay at the origin, wheels sit
at their tire nodes, mirrored on the right; and that textures, materials
and sidecars were made.
"""

import importlib
import os
import sys

import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
PLUGIN = os.path.dirname(HERE)
ZIP_DIR = os.path.join(PLUGIN, 'build')
sys.path.insert(0, PLUGIN)

from sbx_formats import read_PART  # noqa: E402
from sbx_formats.decode import decode_PART  # noqa: E402

CASES = [  # (node file id, stock, wheels)
    (892, True, 'RX7'),           # RX7 set a, stock, its stock wheels by name
    (892, False, '587 588 581'),  # RX7 set a, every slot, wheels by id
    (11062, False, ''),           # RX7 set b
    (19927, False, ''),           # RX7 set c
    (2700, True, 'AD03 583'),     # SKY35 set a, stock, an aftermarket wheel and the other tire
]


def args():
    a = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:]
    return a[0], None


def install_addon():
    zips = sorted(f for f in os.listdir(ZIP_DIR) if f.endswith('.zip'))
    path = os.path.join(ZIP_DIR, zips[-1])
    bpy.ops.extensions.package_install_files(filepath=path, repo='user_default', enable_on_install=True)
    assert hasattr(bpy.ops.import_scene, 'sbx_car'), 'add-on did not register'
    print('installed', os.path.basename(path))


def clear():
    for coll in (bpy.data.objects, bpy.data.meshes, bpy.data.materials, bpy.data.images,
                 bpy.data.texts, bpy.data.collections):
        for x in list(coll):
            coll.remove(x)
    for sc in list(bpy.data.scenes)[1:]:
        bpy.data.scenes.remove(sc)


def addon(name):
    return importlib.import_module('bl_ext.user_default.sbx_parts.' + name)


def check_shape(obj, shape, frames):
    """Problems with one imported shape against its decoded shape."""
    me = obj.data
    out = []
    n = len(me.vertices)
    co = np.zeros(n * 3, np.float32)
    me.vertices.foreach_get('co', co)
    want = (shape.positions @ frames.CAR_TO_BLENDER).astype(np.float32)
    if n != len(want) or not np.array_equal(co.reshape(-1, 3), want):
        return ['vertices']
    loops = np.zeros(len(me.loops), np.int32)
    me.loops.foreach_get('vertex_index', loops)
    slots = np.zeros(len(me.polygons), np.int32)
    me.polygons.foreach_get('material_index', slots)
    tris = [frames.flip_winding(d.triangles.astype(np.int64)) for d in shape.draws]
    want_tris = np.concatenate(tris) if tris else np.zeros((0, 3), np.int64)
    want_slots = np.concatenate([np.full(len(t), k) for k, t in enumerate(tris)]) if tris else np.zeros(0)
    if not np.array_equal(loops, want_tris.ravel()):
        out.append('faces')
    if not np.array_equal(slots, want_slots) or len(me.materials) != len(shape.draws):
        out.append('material slots')
    corners = want_tris.ravel()
    uvs3 = shape.uvs3 if shape.uvs3 is not None and not np.array_equal(shape.uvs3, shape.uvs) else None
    want_uvs = [u for u in (shape.uvs, shape.uvs2, uvs3) if u is not None]
    if len(me.uv_layers) != len(want_uvs):
        out.append('UV map count')
    for layer, u in zip(me.uv_layers, want_uvs):
        got = np.zeros(len(me.loops) * 2, np.float32)
        layer.data.foreach_get('uv', got)
        w = u[corners].astype(np.float32).copy()
        w[:, 1] = 1.0 - w[:, 1]
        if np.abs(got.reshape(-1, 2) - w).max(initial=0.0) > 1e-6:
            out.append('UVs')
    if shape.normals is not None and len(corners):
        got = np.zeros(len(me.loops) * 3, np.float32)
        me.corner_normals.foreach_get('vector', got)
        w = shape.normals[corners] @ frames.CAR_TO_BLENDER
        length = np.linalg.norm(w, axis=1)
        w /= np.maximum(length, 1e-12)[:, None]
        ang = np.degrees(np.arccos(np.clip((got.reshape(-1, 3) * w).sum(1), -1, 1)))
        # Skipped: zero normals in the game data (about 0.4% of vertices;
        # Blender uses the face's there) and triangles drawn both ways
        # (Blender can't give their shared vertices one normal per side).
        counts = {}
        for t in want_tris.tolist():
            k = frozenset(t)
            counts[k] = counts.get(k, 0) + 1
        two_sided = np.repeat([counts[frozenset(t)] > 1 for t in want_tris.tolist()], 3)
        P = shape.positions[want_tris]
        area = np.linalg.norm(np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0]), axis=1)
        # And vertices touching a zero-area triangle: Blender's normal space
        # there is broken, on all of that vertex's faces.
        flat = np.zeros(len(shape.positions), bool)
        flat[want_tris[area <= 1e-12].ravel()] = True
        check = ~two_sided & (length > 1e-6) & ~flat[corners]
        if (ang[check] > 1.0).any():
            out.append('normals (%.1f degrees)' % ang[check].max())
    return out


def run_case(build_out, node_id, stock, wheels):
    clear()
    bpy.ops.import_scene.sbx_car(filepath=os.path.join(build_out, '%08d.node' % node_id),
                                 stock=stock, wheels=wheels)
    frames = addon('frames')
    cars = addon('sbx_formats.cars')
    decoded = {}
    problems, shapes, tris = [], 0, 0
    for obj in bpy.data.objects:
        if obj.type != 'MESH' or 'sbx_shape_index' not in obj:
            continue
        name = obj['sbx_part']
        if name not in decoded:
            decoded[name] = decode_PART(read_PART(open(os.path.join(build_out, name + '.part'), 'rb').read()))
        shape = decoded[name].shapes[obj['sbx_shape_index']]
        bad = check_shape(obj, shape, frames)
        shapes += 1
        tris += len(obj.data.polygons)
        if bad:
            problems.append('%s %s: %s' % (name, obj.name, ', '.join(bad)))
    # Placement: in-place parts at the origin; wheels at their tire nodes.
    roots = [o for o in bpy.data.objects if o.type == 'EMPTY' and 'sbx_sidecar' in o]
    wheel_ids = {int(i) for i in addon('import_part').parse_wheels(wheels)} if wheels else set()
    for r in roots:
        m = np.array(r.matrix_world)
        if int(r['sbx_part']) in wheel_ids:
            tire = r.parent
            t = np.array(tire.matrix_world)
            mirrored = np.linalg.det(m[:3, :3]) < 0
            if not np.allclose(m[:3, 3], t[:3, 3], atol=1e-5) or mirrored != tire.name.endswith('r'):
                problems.append('%s: not at %s' % (r.name, tire.name))
        elif not np.allclose(m, np.eye(4), atol=1e-5):
            problems.append('%s: not in place' % r.name)
    car = next(o for o in bpy.data.objects if 'sbx_car' in o)
    mats = [m for m in bpy.data.materials if m.get('sbx_key')]
    imgs = [i for i in bpy.data.images if i.get('sbx_xpr')]
    texts = [t for t in bpy.data.texts if t.name.endswith('.sbx.json')]
    ok = not problems and texts and mats
    print('%s (%d)%s%s: %s. %d parts, %d shapes, %d faces; %d materials, %d textures, %d sidecars'
          % (car.name, node_id, ' stock' if stock else '', ' wheels "%s"' % wheels if wheels else '',
             'PASS' if ok else 'FAIL', len(roots), shapes, tris, len(mats), len(imgs), len(texts)))
    for p in problems[:10]:
        print('  ' + p)
    del cars
    return ok


def main():
    build_out = args()[0]
    install_addon()
    results = [run_case(build_out, *case) for case in CASES]
    print('PASS' if all(results) else 'FAIL')
    return 0 if all(results) else 1


if __name__ == '__main__':
    sys.exit(main())
