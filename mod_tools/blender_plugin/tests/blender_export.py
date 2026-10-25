#!/usr/bin/env python3
"""Blender export test (milestones 6 and 7).

    blender -b --factory-startup --python tests/blender_export.py -- BUILD_OUT

(or run with a Python that has the bpy module.) Installs the add-on zip
from build/ (run build.py first), then:

1. imports cars (CASES), exports every visible part, and compares each
   written file with the game's: all must be byte-identical;
2. hides everything but one part: only that part is written;
3. edits (milestone 7): moves vertices, subdivides, deletes faces, puts
   another part's material on faces, edits the readable sidecar, adds a
   modifier with Apply modifiers on and off; each export must read back
   with the edit in it and the rest unchanged. Edits that can't be
   written (stream type 0x0D, a missing tire shape key, a
   shader the part doesn't have) must be refused, naming the shape. Also
   the tire morphs (shape keys), a normals-only edit and a moved marker;
4. imports again, saves the .blend, opens it and exports: identical.
"""

import os
import shutil
import sys
import tempfile

import json

import bpy
import bmesh  # noqa: E402 (after bpy: the bpy module provides it)
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import blender_import as T  # noqa: E402  (install_addon, clear, args)

CASES = [  # (node file id, stock, wheels)
    (892, False, (587, 588, 581)),    # RX7 set a, every slot, wheels
    (11062, False, ()),                # RX7 set b
    (19927, False, ()),                # RX7 set c
    (2700, True, ()),                  # SKY35 set a, stock
]


def export(folder, skip_identical=False, apply_modifiers=False):
    if os.path.exists(folder):
        shutil.rmtree(folder)
    return bpy.ops.export_scene.sbx_parts(directory=folder, skip_identical=skip_identical,
                                          apply_modifiers=apply_modifiers)


def compare_folder(folder, build_out):
    names = sorted(os.listdir(folder))
    bad = [n for n in names if open(os.path.join(folder, n), 'rb').read() !=
           open(os.path.join(build_out, n), 'rb').read()]
    return names, bad


def roots():
    return [o for o in bpy.data.objects if o.type == 'EMPTY' and 'sbx_sidecar' in o]


def read_back(path):
    from sbx_formats import read_PART
    from sbx_formats.decode import decode_PART
    from sbx_formats.part_write import write_PART
    d = open(path, 'rb').read()
    part = read_PART(d)
    assert write_PART(part) == d, 'the written file doesn\'t re-write identically'
    return part, decode_PART(part)


def try_export(folder, **kw):
    try:
        r = export(folder, **kw)
        return r, ''
    except RuntimeError as e:   # the operator's ERROR report raises in background mode
        return {'CANCELLED'}, str(e).strip().splitlines()[-1]


def tri_count(dp):
    return sum(len(d.triangles) for s in dp.shapes for d in s.draws)


def reimport_sky35(build_out):
    T.clear()
    bpy.ops.import_scene.sbx_car(filepath=os.path.join(build_out, '00002700.node'), stock=True)
    return next(o for o in roots() if o['sbx_part'] == '00002844')


def edit_tests(build_out, tmp, bonnet):
    ok = True
    orig_part, orig = read_back(os.path.join(build_out, '00002844.part'))

    def report(what, good, detail):
        nonlocal ok
        ok &= bool(good)
        print('%s: %s (%s)' % (what, 'PASS' if good else 'FAIL', detail))

    def shape_obj(root):
        return next(c for c in root.children if c.type == 'MESH')

    # Move every vertex up 1 cm.
    bonnet = reimport_sky35(build_out)
    obj = shape_obj(bonnet)
    for v in obj.data.vertices:
        v.co.z += 0.01
    out = os.path.join(tmp, 'm1')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00002844.part'))
    a, b = orig.shapes[0], dp.shapes[0]
    dy = b.positions[:, 1] - a.positions[:, 1] if len(a.positions) == len(b.positions) else np.array([1.0])
    good = (r == {'FINISHED'} and np.allclose(dy, 0.01, atol=1e-6)
            and np.allclose(b.positions[:, [0, 2]], a.positions[:, [0, 2]])
            and tri_count(dp) == tri_count(orig) and np.array_equal(a.uvs, b.uvs)
            and [x.raw[:0x158] for x in part.shapes[0].draws] == [x.raw[:0x158] for x in orig_part.shapes[0].draws]
            and part.shapes[0].get('bounds_box') != orig_part.shapes[0].get('bounds_box'))
    report('moved up 1 cm', good, '%d vertices, y +%.4f m, %d triangles, UVs and records the same, bounds updated'
           % (len(b.positions), float(dy.mean()), tri_count(dp)))

    # Subdivide: more vertices and faces.
    bonnet = reimport_sky35(build_out)
    obj = shape_obj(bonnet)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=1, use_grid_fill=True)
    bm.to_mesh(obj.data)
    bm.free()
    out = os.path.join(tmp, 'm2')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00002844.part'))
    good = r == {'FINISHED'} and tri_count(dp) > 3 * tri_count(orig) and len(dp.shapes[0].positions) > len(orig.shapes[0].positions)
    report('subdivided', good, '%d -> %d vertices, %d -> %d triangles' % (
        len(orig.shapes[0].positions), len(dp.shapes[0].positions), tri_count(orig), tri_count(dp)))

    # Delete half the faces.
    bonnet = reimport_sky35(build_out)
    obj = shape_obj(bonnet)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.delete(bm, geom=list(bm.faces)[::2], context='FACES_ONLY')
    bm.to_mesh(obj.data)
    bm.free()
    out = os.path.join(tmp, 'm3')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00002844.part'))
    good = r == {'FINISHED'} and abs(tri_count(dp) - tri_count(orig) / 2) <= 1
    report('half the faces deleted', good, '%d -> %d triangles' % (tri_count(orig), tri_count(dp)))

    # Another part's material on the first 100 faces (its shader pair must be in the bonnet part).
    bonnet = reimport_sky35(build_out)
    obj = shape_obj(bonnet)
    pair_ids = json.loads(bpy.data.texts['00002844.sbx.json'].as_string())['shader_pairs']
    ids = {p['shader'] for p in pair_ids}
    own = {s.material.name for s in obj.material_slots}
    other = next(m for m in bpy.data.materials if m.get('sbx_record') and m.name not in own
                 and m.get('sbx_shader') and m['sbx_record'].split(':')[0] != '00002844'
                 and shader_ids(m) in ids)
    obj.data.materials.append(other)
    for p in obj.data.polygons[:100]:
        p.material_index = len(obj.material_slots) - 1
    out = os.path.join(tmp, 'm4')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00002844.part'))
    good = r == {'FINISHED'} and len(dp.shapes[0].draws) == len(orig.shapes[0].draws) + 1 \
        and len(dp.shapes[0].draws[-1].triangles) == 100 and tri_count(dp) == tri_count(orig)
    report('another part\'s material on 100 faces', good, 'draws %d -> %d (%s, last draw %d triangles)' % (
        len(orig.shapes[0].draws), len(dp.shapes[0].draws), other.name, len(dp.shapes[0].draws[-1].triangles)))

    # Suzanne joined into the bonnet: more vertex data; the header's
    # vertex data size must follow (the game copies that many bytes).
    bonnet = reimport_sky35(build_out)
    obj = shape_obj(bonnet)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.create_monkey(bm)
    bm.to_mesh(obj.data)
    bm.free()
    out = os.path.join(tmp, 'm12')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00002844.part'))
    size = sum(st.size for s in part.shapes for st in s.streams if st is not None) + \
        sum(d.get('index_size') for s in part.shapes for d in s.draws)
    good = (r == {'FINISHED'} and part.header.get('vertex_data_size') == size
            and size > orig_part.header.get('vertex_data_size') and tri_count(dp) == tri_count(orig) + 968)
    report('Suzanne joined in', good, 'vertex data %d -> %d bytes, header says %d; %d -> %d triangles' % (
        orig_part.header.get('vertex_data_size'), size, part.header.get('vertex_data_size'),
        tri_count(orig), tri_count(dp)))

    # A material whose shader this part doesn't have: the pair comes in
    # from the part the material was imported with (add-on 1.1).
    bonnet = reimport_sky35(build_out)
    obj = shape_obj(bonnet)
    foreign = next((m for m in bpy.data.materials if m.get('sbx_shader') and shader_ids(m) not in ids), None)
    if foreign is not None:
        obj.data.materials.append(foreign)
        obj.data.polygons[0].material_index = len(obj.material_slots) - 1
        out = os.path.join(tmp, 'm5')
        r, msg = try_export(out)
        if r != {'FINISHED'}:
            report('material with a shader the part lacks', False, msg[:120])
        else:
            from sbx_formats import shaders
            part, dp = read_back(os.path.join(out, '00002844.part'))
            _, pairs, _ = shaders.split_section(part.shaders_raw)
            last = part.shapes[0].draws[-1]
            good = (len(pairs) == orig_part.shader_count + 1 and pairs[last.get('shader')].ids == shader_ids(foreign)
                    and len(dp.shapes[0].draws[-1].triangles) == 1)
            report('material with a shader the part lacks', good, 'pair %s brought in as slot %d' % (
                shader_ids(foreign), last.get('shader')))

    # Readable sidecar edit: specular of the bonnet's first draw.
    bonnet = reimport_sky35(build_out)
    t = bpy.data.texts['00002844.sbx.json']
    side = json.loads(t.as_string())
    side['shapes'][0]['draws'][0]['specular'] = [0.25, 0.5, 0.75, 1.0]
    t.clear()
    t.write(json.dumps(side, indent=2))
    out = os.path.join(tmp, 'm6')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00002844.part'))
    rec, was = part.shapes[0].draws[0], orig_part.shapes[0].draws[0]
    changed = [k for k in range(0x200) if rec.raw[k] != was.raw[k]]
    good = (r == {'FINISHED'} and tuple(rec.get('specular')) == (0.25, 0.5, 0.75, 1.0)
            and all(0x28 <= k < 0x38 for k in changed)
            and part.shapes[0].streams[1].raw.tobytes() == orig_part.shapes[0].streams[1].raw.tobytes())
    report('specular edited in the readable sidecar', good, 'record bytes changed: %s; geometry untouched'
           % ', '.join('0x%X' % k for k in changed))

    # A Subdivision Surface modifier: ignored, then applied.
    bonnet = reimport_sky35(build_out)
    obj = shape_obj(bonnet)
    obj.modifiers.new('subsurf', 'SUBSURF').levels = 1
    out = os.path.join(tmp, 'm7')
    r, msg = try_export(out)
    same = open(os.path.join(out, '00002844.part'), 'rb').read() == open(os.path.join(build_out, '00002844.part'), 'rb').read()
    out = os.path.join(tmp, 'm8')
    r2, msg2 = try_export(out, apply_modifiers=True)
    part, dp = read_back(os.path.join(out, '00002844.part'))
    report('modifier', r == {'FINISHED'} and same and r2 == {'FINISHED'} and tri_count(dp) > tri_count(orig),
           'off: identical to the game\'s; on: %d -> %d triangles' % (tri_count(orig), tri_count(dp)))

    # A window with stream type 0x10 (color, UV, UV2, UV3 = UV1): editable.
    T.clear()
    bpy.ops.import_scene.sbx_part(filepath=os.path.join(build_out, '00006280.part'))
    win = next(o for o in bpy.data.objects if o.get('sbx_shape_name') == 'window01_Shape')
    nuv = len(win.data.uv_layers)
    win.data.vertices[0].co.z += 0.01
    out = os.path.join(tmp, 'm9')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00006280.part'))
    _, was = read_back(os.path.join(build_out, '00006280.part'))
    k = [s.name for s in dp.shapes].index('window01_Shape')
    w, w0 = dp.shapes[k], was.shapes[k]
    good = (r == {'FINISHED'} and nuv == 2 and part.shapes[k].streams[0].type == 0x10
            and np.array_equal(w.uvs, w0.uvs) and np.array_equal(w.uvs2, w0.uvs2) and np.array_equal(w.uvs3, w0.uvs)
            and abs(w.positions[0, 1] - w0.positions[0, 1] - 0.01) < 1e-6)
    report('window with stream type 0x10 edited', good, '%d UV maps in Blender; UV, UV2 kept, UV3 = UV1, vertex moved' % nuv)
    # The bonnet with stream type 0x0D (part 2618): refused.
    T.clear()
    bpy.ops.import_scene.sbx_part(filepath=os.path.join(build_out, '00002618.part'))
    bn = next(o for o in bpy.data.objects if o.get('sbx_shape_name') == 'bnmg00_Shape')
    bn.data.vertices[0].co.z += 0.01
    r, msg = try_export(os.path.join(tmp, 'm9b'))
    report('bonnet with stream type 0x0D refused', r == {'CANCELLED'} and bn.name in msg and '0x0D' in msg, msg[:120])
    bonnet = reimport_sky35(build_out)
    win = next(o for o in bpy.data.objects if o.get('sbx_shape_name') == 'window01_Shape')
    win.data.vertices[0].co.z += 0.01
    out = os.path.join(tmp, 'm11')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00002890.part'))
    _, was = read_back(os.path.join(build_out, '00002890.part'))
    w, w0 = dp.shapes[1], was.shapes[1]
    report('SKY35 window (stream type 0x0F) edited', r == {'FINISHED'} and np.array_equal(w.uvs2, w0.uvs2)
           and abs(w.positions[0, 1] - w0.positions[0, 1] - 0.01) < 1e-6, 'second UV map kept, vertex moved')
    # Tire morphs: edit base_Shape and a shape key, then subdivide.
    def tire():
        T.clear()
        bpy.ops.import_scene.sbx_part(filepath=os.path.join(build_out, '00000583.part'))
        return next(o for o in bpy.data.objects if o.get('sbx_shape_name') == 'base_Shape')
    from sbx_formats.decode import morph_groups
    _, tire_orig = read_back(os.path.join(build_out, '00000583.part'))
    obj = tire()
    for v in obj.data.vertices:
        v.co.x *= 1.05
    key = obj.data.shape_keys.key_blocks['target_02_Shape']
    for d in key.data:
        d.co.z += 0.02
    out = os.path.join(tmp, 't1')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00000583.part'))
    names = [s.name for s in dp.shapes]
    base, t2 = dp.shapes[names.index('base_Shape')], dp.shapes[names.index('target_02_Shape')]
    obase, ot2 = tire_orig.shapes[names.index('base_Shape')], tire_orig.shapes[names.index('target_02_Shape')]
    groups = morph_groups(dp.shapes)
    good = (r == {'FINISHED'} and len(groups.get(names.index('base_Shape'), [])) == 5
            and np.allclose(base.positions[:, 2], obase.positions[:, 2] * 1.05, atol=1e-5)
            and np.allclose(t2.positions[:, 1] - ot2.positions[:, 1], 0.02, atol=1e-5))
    report('tire: base and a shape key edited', good, 'base widened 5%%, target_02 up 2 cm, %d targets still morph targets'
           % len(groups.get(names.index('base_Shape'), [])))
    obj = tire()
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=1, use_grid_fill=True)
    bm.to_mesh(obj.data)
    bm.free()
    out = os.path.join(tmp, 't2')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00000583.part'))
    names = [s.name for s in dp.shapes]
    groups = morph_groups(dp.shapes)
    counts = {len(s.positions) for s in dp.shapes if s.name == 'base_Shape' or s.name.startswith('target_')}
    good = r == {'FINISHED'} and len(groups.get(names.index('base_Shape'), [])) == 5 and len(counts) == 1
    report('tire subdivided', good, 'base and all 5 targets %s vertices' % counts)
    obj = tire()
    obj.shape_key_remove(obj.data.shape_keys.key_blocks['target_04_Shape'])
    obj.data.vertices[0].co.z += 0.01
    r, msg = try_export(os.path.join(tmp, 't3'))
    report('tire with a shape key deleted refused', r == {'CANCELLED'} and 'target_04_Shape' in msg, msg[:120])

    # Normals only: flat custom normals on the bonnet, nothing moved.
    bonnet = reimport_sky35(build_out)
    obj = shape_obj(bonnet)
    obj.data.normals_split_custom_set([(0.0, 0.0, 1.0)] * len(obj.data.loops))
    out = os.path.join(tmp, 'n1')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00002844.part'))
    nz = dp.shapes[0].normals[:, 1]
    good = r == {'FINISHED'} and np.allclose(nz, 1.0, atol=1e-3) and np.array_equal(dp.shapes[0].positions, orig.shapes[0].positions)
    report('normals only edited', good, 'all normals now car-space up; positions unchanged')

    # A marker moved: the headlight's high-beam glow point, 10 cm forward.
    reimport_sky35(build_out)
    mk = next(o for o in bpy.data.objects if o.get('sbx_marker') == 'hl00ef_h00')
    mk.location.y -= 0.1          # Blender -y is the car's front
    out = os.path.join(tmp, 'k1')
    r, msg = try_export(out)
    part, dp = read_back(os.path.join(out, '00002882.part'))
    hl_orig, _ = read_back(os.path.join(build_out, '00002882.part'))
    new_nodes, old_nodes = part.embedded_nodes.nodes, hl_orig.embedded_nodes.nodes
    k = next(i for i, n in enumerate(old_nodes) if n.name == 'hl00ef_h00')
    others_same = all(np.array_equal(a.translation, b.translation) and np.array_equal(a.inverse_bind, b.inverse_bind)
                      for j, (a, b) in enumerate(zip(new_nodes, old_nodes)) if j != k)
    d = new_nodes[k].translation - old_nodes[k].translation
    good = r == {'FINISHED'} and np.allclose(d, [0, 0, -1.0], atol=1e-4) and others_same \
        and all(a.raw == b.raw for a, b in zip(part.shapes, hl_orig.shapes))
    report('marker moved', good, 'hl00ef_h00 moved %s in the node frame (decimeters, z to the rear); '
           'other nodes and the shapes unchanged' % np.round(d, 4).tolist())
    return ok


def shader_ids(mat):
    """A material's shader as ids, from its sbx_shader hashes."""
    from sbx_formats.shaders import shader_id
    vs, ps = mat['sbx_shader'].split(':')
    return '%s + %s' % (shader_id(vs, 'vs'), shader_id(ps, 'ps'))


def main():
    build_out = T.args()[0]
    T.install_addon()
    tmp = tempfile.mkdtemp()
    ok = True

    for node_id, stock, wheels in CASES:
        T.clear()
        bpy.ops.import_scene.sbx_car(filepath=os.path.join(build_out, '%08d.node' % node_id),
                                     stock=stock, wheels=' '.join(map(str, wheels)))
        out = os.path.join(tmp, 'out')
        r = export(out)
        names, bad = compare_folder(out, build_out) if r == {'FINISHED'} else ([], ['export failed'])
        good = r == {'FINISHED'} and not bad and len(names) == len({o['sbx_part'] for o in roots()})
        ok &= good
        print('%d%s: %s, %d parts written, %d differ from the game\'s'
              % (node_id, ' stock' if stock else '', 'PASS' if good else 'FAIL', len(names), len(bad)))

    # 2. Hide all but the bonnet (SKY35, still loaded).
    bonnet = next(o for o in roots() if o['sbx_part'] == '00002844')
    for o in bpy.data.objects:
        if o.type == 'MESH':
            o.hide_set(o.parent is not bonnet)
    out = os.path.join(tmp, 'bonnet')
    export(out)
    names = sorted(os.listdir(out))
    good = names == ['00002844.part']
    ok &= good
    print('only the bonnet visible: %s, written %s' % ('PASS' if good else 'FAIL', names))
    for o in bpy.data.objects:
        o.hide_set(False)

    # 3. Edits are written (milestone 7) and read back right.
    ok &= edit_tests(build_out, tmp, bonnet)
    # 4. Import again, save, open again, export.
    reimport_sky35(build_out)
    blend = os.path.join(tmp, 'sky35.blend')
    bpy.ops.wm.save_as_mainfile(filepath=blend)
    bpy.ops.wm.open_mainfile(filepath=blend)
    out = os.path.join(tmp, 'reopened')
    export(out, skip_identical=False)
    names, bad = compare_folder(out, build_out)
    good = names and not bad
    ok &= good
    print('saved and reopened: %s, %d written, %d differ' % ('PASS' if good else 'FAIL', len(names), len(bad)))
    out = os.path.join(tmp, 'skip')
    export(out, skip_identical=True)
    good = not os.path.exists(out) or not os.listdir(out)
    ok &= good
    print('skip unchanged parts: %s' % ('PASS, nothing written' if good else 'FAIL'))
    print('PASS' if ok else 'FAIL')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
