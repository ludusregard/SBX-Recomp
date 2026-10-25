"""Blender test: adding shapes, markers and materials to parts (add-on 1.1).

    blender -b --factory-startup --python tests/blender_add.py -- BUILD_OUT

Installs the newest zip from build/, builds a shader library from
BUILD_OUT into a temporary file, then, each case on a fresh import:

1. S30 side skirt (part 5757, set a): a copy of ssmg01_Shape, 5 cm up,
   joined as ssmg02_Shape with Add to Part. The written part has both
   shapes, the new one where it was put, a shape node and an embedded
   node for it, and follows the game's rules (compose.check_rules); the
   readable sidecar lists it under new_shapes.
2. A Suzanne named aa_new_Shape joined to the same part: it sorts first,
   so the old shape moves to index 1 and its node follows.
3. A new marker (an empty) joined to the part: written at its position.
4. Make SBX Material with a shader pair no imported part has (from the
   library): the pair is appended to the part's shader section and the
   new shape's draw uses it, with the material's own diffuse color.
5. The bonnet (SKY35 2844) with an imported material from another part
   in a slot, faces unchanged: the draw takes that material's record
   (before 1.1 this went unnoticed).
6. RX7 set b meters: the needles (sm_hari00_Shape ...) hang from their
   markers. Rotating a marker exports identical to the game's file;
   moving it moves the marker and the needle together.
7. Remove from Part: the part exports identical to the game's again.
"""

import json
import os
import sys
import tempfile

import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..'))
import blender_import as T  # noqa: E402
from compose import record_body  # noqa: E402  (a record without its index placement)

from sbx_formats import read_PART, shaders  # noqa: E402
from sbx_formats.compose import check_rules  # noqa: E402
from sbx_formats.decode import decode_PART  # noqa: E402
from sbx_formats import library  # noqa: E402

ok = True


def report(what, good, detail=''):
    global ok
    ok &= bool(good)
    print('%s: %s%s' % (what, 'PASS' if good else 'FAIL', ' (%s)' % detail if detail else ''))


def export(folder, **kw):
    try:
        return bpy.ops.export_scene.sbx_parts(directory=folder, **kw), ''
    except RuntimeError as e:
        return {'CANCELLED'}, str(e).strip().splitlines()[-1]


def read(path):
    d = open(path, 'rb').read()
    p = read_PART(d)
    return p, decode_PART(p), d


def root_of(part_name):
    return next(o for o in bpy.data.objects if o.type == 'EMPTY' and 'sbx_sidecar' in o and o['sbx_part'] == part_name)


def shape_of(root, name):
    return next(o for o in root.children_recursive if o.get('sbx_shape_name') == name)


def select(objs, active):
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    for o in objs:
        o.select_set(True)
    active.select_set(True)
    bpy.context.view_layer.objects.active = active


def duplicate(obj, name):
    new = obj.copy()
    new.data = obj.data.copy()
    for k in [k for k in new.keys() if k.startswith('sbx_')]:
        del new[k]
    new.name = name
    obj.users_collection[0].objects.link(new)
    new.parent = None
    new.matrix_world = obj.matrix_world.copy()
    return new


def set_draw_value(part_name, shape_name, slot, **values):
    t = bpy.data.texts['%s.sbx.json' % part_name]
    side = json.loads(t.as_string())
    e = next(x for x in side['shapes'] if x['name'] == shape_name)
    d = next(x for x in e['draws'] if x.get('slot') == slot)
    d.update(values)
    t.clear()
    t.write(json.dumps(side, indent=2))


def import_s30():
    T.clear()
    bpy.ops.import_scene.sbx_car(filepath=os.path.join(BUILD_OUT, '00005638.node'), stock=True)
    return root_of('00005757')


def main():
    global BUILD_OUT
    BUILD_OUT = T.args()[0]
    T.install_addon()
    tmp = tempfile.mkdtemp()
    lib_path = os.path.join(tmp, 'shader_library.json')
    library.write_library(library.build_library(BUILD_OUT, read_PART), lib_path)
    bpy.context.preferences.addons['bl_ext.user_default.sbx_parts'].preferences.library_path = lib_path
    orig, orig_dp, orig_bytes = read(os.path.join(BUILD_OUT, '00005757.part'))

    # 1. A second side skirt shape.
    root = import_s30()
    src = shape_of(root, 'ssmg01_Shape')
    new = duplicate(src, 'ssmg02_Shape')
    new.location.z += 0.05
    bpy.context.view_layer.update()
    select([new], root)
    r = bpy.ops.sbx.add_to_part()
    side = json.loads(bpy.data.texts['00005757.sbx.json'].as_string())
    out = os.path.join(tmp, 'a1')
    r2, msg = export(out)
    if r2 != {'FINISHED'}:
        report('second side skirt shape', False, msg)
    else:
        p, dp, _ = read(os.path.join(out, '00005757.part'))
        names = [s.name for s in p.shapes]
        a, b = dp.shapes[0], dp.shapes[1] if len(dp.shapes) > 1 else None
        good = (r == {'FINISHED'} and names == ['ssmg01_Shape', 'ssmg02_Shape'] and not check_rules(p)
                and b is not None and np.allclose(b.positions[:, 1] - a.positions[:, 1], 0.05, atol=1e-5)
                and np.allclose(b.positions[:, [0, 2]], a.positions[:, [0, 2]], atol=1e-5)
                and [n.name for n in p.nodes] == ['ss', 'ss'] and len(p.embedded_nodes.nodes) == 2
                and any(e.get('new') and e['name'] == 'ssmg02_Shape' for e in side['shapes'])
                and new.parent is root and new.get('sbx_new_shape') == 'ssmg02_Shape')
        report('second side skirt shape (ssmg02_Shape joined to 00005757_ss)', good,
               'shapes %s, nodes %s, embedded %d, new one %.3f m up; sidecar shapes marked new %s' % (
                   names, [n.name for n in p.nodes], len(p.embedded_nodes.nodes),
                   float((b.positions[:, 1] - a.positions[:, 1]).mean()) if b is not None else 0,
                   [e['name'] for e in side['shapes'] if e.get('new')]))

    # 2. A new shape that sorts first.
    root = import_s30()
    bpy.ops.mesh.primitive_monkey_add(size=0.3, location=(1.5, 0, 0.5))
    monkey = bpy.context.active_object
    monkey.name = 'aa_new_Shape'
    monkey.data.materials.append(shape_of(root, 'ssmg01_Shape').material_slots[0].material)
    select([monkey], root)
    bpy.ops.sbx.add_to_part()
    out = os.path.join(tmp, 'a2')
    r, msg = export(out)
    if r != {'FINISHED'}:
        report('new shape sorting first', False, msg)
    else:
        p, dp, _ = read(os.path.join(out, '00005757.part'))
        shape_nodes = sorted(n.get('shape_index') for n in p.nodes)
        good = ([s.name for s in p.shapes] == ['aa_new_Shape', 'ssmg01_Shape'] and not check_rules(p)
                and shape_nodes == [0, 1] and len(dp.shapes[0].positions) >= 507
                and np.array_equal(p.shapes[1].streams[1].raw, orig.shapes[0].streams[1].raw))
        report('new shape sorting first (aa_new_Shape)', good, 'shapes %s, %d vertices, old shape\'s data kept'
               % ([s.name for s in p.shapes], len(dp.shapes[0].positions)))

    # 3. A new marker.
    root = import_s30()
    bpy.ops.object.empty_add(location=(0.9, 0.5, 0.2))
    mk = bpy.context.active_object
    mk.name = 'ss00_test_ef'
    select([mk], root)
    bpy.ops.sbx.add_to_part()
    out = os.path.join(tmp, 'a3')
    r, msg = export(out)
    if r != {'FINISHED'}:
        report('new marker', False, msg)
    else:
        p, dp, _ = read(os.path.join(out, '00005757.part'))
        m = next((x for x in dp.markers if x.name == 'ss00_test_ef'), None)
        frames = T.addon('frames')
        want = np.array([0.9, 0.5, 0.2]) @ np.linalg.inv(frames.NODE_TO_BLENDER)
        good = m is not None and np.allclose(m.position, want, atol=1e-4) and not check_rules(p) \
            and np.array_equal(p.shapes[0].streams[1].raw, orig.shapes[0].streams[1].raw)
        report('new marker', good, 'ss00_test_ef at %s in the node frame' % (
            None if m is None else np.round(m.position, 3).tolist()))

    # 4. A new material with a shader pair from the library.
    root = import_s30()
    src_obj = shape_of(root, 'ssmg01_Shape')
    sc_pairs = set()
    for t in bpy.data.texts:
        if t.name.endswith('.sbx.json'):
            for e in json.loads(t.as_string())['shader_pairs']:
                sc_pairs.add(e['vs_hash'] + ':' + e['ps_hash'])
    lib = library.load_library(lib_path)
    key = next((k for k, e in sorted(lib['pairs'].items(), key=lambda kv: kv[1]['ids'])
                if k not in sc_pairs and any(ex['key'][1] == 1 for ex in e['examples'])), None)
    if key is None:
        report('new material from the library', False, 'every library pair is already imported')
    else:
        ex = next(i for i, x in enumerate(lib['pairs'][key]['examples']) if x['key'][1] == 1)
        new = duplicate(src_obj, 'ssmg03_Shape')
        new.data.materials.clear()
        mat = bpy.data.materials.new('chrome test')
        new.data.materials.append(mat)
        select([new], root)
        bpy.ops.sbx.add_to_part()
        bpy.context.view_layer.objects.active = new
        r = bpy.ops.sbx.make_material(material='chrome test', pair=key, template='library:%s:%d' % (key, ex))
        set_draw_value('00005757', 'ssmg03_Shape', 1, diffuse=[0.2, 0.4, 0.6, 1.0])
        out = os.path.join(tmp, 'a4')
        r2, msg = export(out)
        if r2 != {'FINISHED'}:
            report('new material from the library', False, msg)
        else:
            p, dp, _ = read(os.path.join(out, '00005757.part'))
            _, pairs, _ = shaders.split_section(p.shaders_raw)
            k = next(i for i, s in enumerate(p.shapes) if s.name == 'ssmg03_Shape')
            rec = p.shapes[k].draws[0]
            good = (r == {'FINISHED'} and len(pairs) == orig.shader_count + 1
                    and pairs[rec.get('shader')].vs_hash + ':' + pairs[rec.get('shader')].ps_hash == key
                    and np.allclose(rec.get('diffuse'), (0.2, 0.4, 0.6, 1.0)) and not check_rules(p)
                    and record_body(p.shapes[0].draws[0]) == record_body(orig.shapes[0].draws[0]))
            report('new material from the library', good, '%s, pairs %d -> %d, draw uses slot %d' % (
                lib['pairs'][key]['ids'], orig.shader_count, len(pairs), rec.get('shader')))

    # 5. Another imported material in an unedited shape's slot.
    T.clear()
    bpy.ops.import_scene.sbx_car(filepath=os.path.join(BUILD_OUT, '00002700.node'), stock=True)
    bonnet = shape_of(root_of('00002844'), 'bnmg00_Shape')
    was = read(os.path.join(BUILD_OUT, '00002844.part'))[0]
    own = {s.material.name for s in bonnet.material_slots}
    other = next(m for m in bpy.data.materials if m.get('sbx_record') and m.name not in own)
    bonnet.material_slots[0].material = other
    out = os.path.join(tmp, 'a5')
    r, msg = export(out)
    if r != {'FINISHED'}:
        report('material swapped on an unedited shape', False, msg)
    else:
        p, dp, _ = read(os.path.join(out, '00002844.part'))
        rec = p.shapes[0].draws[0]
        orec = was.shapes[0].draws[0]
        good = rec.raw[:0x68] != orec.raw[:0x68] or rec.get('shared_shader') != orec.get('shared_shader') \
            or rec.textures != orec.textures
        report('material swapped on an unedited shape', good, 'slot 1 now %s; record written from it' % other.name)

    # 6. Needles under their markers (RX7 set b meters).
    T.clear()
    bpy.ops.import_scene.sbx_car(filepath=os.path.join(BUILD_OUT, '00011062.node'), stock=False)
    gauges = root_of('00011253')
    needle = shape_of(gauges, 'sm_hari00_Shape')
    marker = needle.parent
    good = marker is not None and marker.get('sbx_marker') == 'sm_hari'
    tm = shape_of(gauges, 'tm_hari00_Shape').parent
    dm = shape_of(root_of('00011250'), 'dm_hari01_Shape').parent
    good = good and tm is not None and tm.get('sbx_marker') == 'tm_hari' and dm is not None \
        and dm.get('sbx_marker') == 'dm_01'
    report('needles under their markers', good, 'sm_hari00 -> %s, tm_hari00 -> %s, dm_hari01 -> %s' % (
        marker.name if marker else None, tm.name if tm else None, dm.name if dm else None))
    if good:
        marker.rotation_euler.y = 0.6
        bpy.context.view_layer.update()
        out = os.path.join(tmp, 'a6')
        r, msg = export(out)
        same = r == {'FINISHED'} and open(os.path.join(out, '00011253.part'), 'rb').read() == \
            open(os.path.join(BUILD_OUT, '00011253.part'), 'rb').read()
        report('marker rotated (needle swing preview): exports unchanged', same, msg)
        marker.rotation_euler.y = 0.0
        marker.location.z += 0.01
        bpy.context.view_layer.update()
        out = os.path.join(tmp, 'a7')
        r, msg = export(out)
        if r != {'FINISHED'}:
            report('marker moved with its needle', False, msg)
        else:
            p, dp, _ = read(os.path.join(out, '00011253.part'))
            _, odp, _ = read(os.path.join(BUILD_OUT, '00011253.part'))
            k = [s.name for s in dp.shapes].index('sm_hari00_Shape')
            dy = dp.shapes[k].positions[:, 1] - odp.shapes[k].positions[:, 1]
            m = next(x for x in dp.markers if x.name == 'sm_hari')
            om = next(x for x in odp.markers if x.name == 'sm_hari')
            good = np.allclose(dy, 0.01, atol=1e-5) and abs((m.position[1] - om.position[1]) - 0.1) < 1e-4
            report('marker moved with its needle', good, 'needle y +%.4f m, marker +%.3f dm' % (
                float(dy.mean()), float(m.position[1] - om.position[1])))

    # 7. Remove from Part.
    root = import_s30()
    new = duplicate(shape_of(root, 'ssmg01_Shape'), 'ssmg02_Shape')
    select([new], root)
    bpy.ops.sbx.add_to_part()
    select([new], new)
    bpy.ops.sbx.remove_from_part()
    out = os.path.join(tmp, 'a8')
    r, msg = export(out)
    same = r == {'FINISHED'} and open(os.path.join(out, '00005757.part'), 'rb').read() == orig_bytes
    side = json.loads(bpy.data.texts['00005757.sbx.json'].as_string())
    report('removed from the part again', same and not any(e.get('new') for e in side['shapes']), msg)

    # 8. Shift+D copy of a shape, joined with Add to Part.
    root = import_s30()
    src = shape_of(root, 'ssmg01_Shape')
    select([src], src)
    bpy.ops.object.duplicate()
    dup = bpy.context.active_object
    dup.name = 'ssmg02_Shape'
    dup.location.z += 0.05
    select([dup], root)
    r = bpy.ops.sbx.add_to_part()
    out = os.path.join(tmp, 'a9')
    r2, msg = export(out)
    names = [s.name for s in read(os.path.join(out, '00005757.part'))[0].shapes] if r2 == {'FINISHED'} else msg
    report('Shift+D copy joined', r == {'FINISHED'} and names == ['ssmg01_Shape', 'ssmg02_Shape'], names)

    # 9. Faces moved to a new material slot in Edit Mode, exported from Edit Mode.
    T.clear()
    bpy.ops.import_scene.sbx_car(filepath=os.path.join(BUILD_OUT, '00002700.node'), stock=True)
    bonnet = shape_of(root_of('00002844'), 'bnmg00_Shape')
    other = next(m for m in bpy.data.materials if m.get('sbx_record') and m.get('sbx_shader')
                 and m.name not in {s.material.name for s in bonnet.material_slots})
    bonnet.data.materials.append(other)
    select([bonnet], bonnet)
    bpy.ops.object.mode_set(mode='EDIT')
    import bmesh
    bm = bmesh.from_edit_mesh(bonnet.data)
    for f in list(bm.faces)[:40]:
        f.material_index = len(bonnet.material_slots) - 1
    bmesh.update_edit_mesh(bonnet.data)
    out = os.path.join(tmp, 'a10')
    r, msg = export(out)
    mode = bonnet.mode
    if r != {'FINISHED'}:
        report('export from Edit Mode', False, msg)
    else:
        p, dp, _ = read(os.path.join(out, '00002844.part'))
        report('export from Edit Mode', len(dp.shapes[0].draws[-1].triangles) == 40 and mode == 'EDIT',
               '%d triangles in the new draw, back in %s mode' % (len(dp.shapes[0].draws[-1].triangles), mode))
    bpy.ops.object.mode_set(mode='OBJECT')

    # 10. The sidecar lists additions in full, and a new draw's values and
    # textures (by name) are edited there.
    root = import_s30()
    new = duplicate(shape_of(root, 'ssmg01_Shape'), 'ssmg02_Shape')
    new.data.materials.clear()
    new.data.materials.append(next(m for m in bpy.data.materials if m.get('sbx_record', '').startswith('00005769')))
    select([new], root)
    bpy.ops.sbx.add_to_part()
    side = json.loads(bpy.data.texts['00005757.sbx.json'].as_string())
    e = next((x for x in side['shapes'] if x.get('new')), {})
    d = (e.get('draws') or [{}])[0]
    full = all(k in e for k in ('shape', 'streams', 'vertices', 'object', 'layout_from')) and \
        all(k in d for k in ('slot', 'material', 'textures', 'triangles', 'shader', 'shader_slot', 'shared_shader',
                             'blend', 'diffuse', 'ambient', 'specular', 'power', 'params', 'flags', 'record_from')) \
        and 'name' in d['textures'][0] and any(n.get('new') for n in side['nodes'])
    report('sidecar lists the new shape in full', full, 'entry keys %s; draw keys %s' % (sorted(e), sorted(d)))
    names = [n for n in root['sbx_xpr_names'].split('\n')]
    want = next(n for n in names if n != d['textures'][0].get('name'))
    set_draw_value('00005757', 'ssmg02_Shape', 1, diffuse=[0.1, 0.2, 0.3, 1.0],
                   textures=[{'xpr': 0, 'index': d['textures'][0]['index'], 'name': want}])
    out = os.path.join(tmp, 'a11')
    r, msg = export(out)
    if r != {'FINISHED'}:
        report('new draw edited in the sidecar', False, msg)
    else:
        p, dp, _ = read(os.path.join(out, '00005757.part'))
        rec = p.shapes[1].draws[0]
        side = json.loads(bpy.data.texts['00005757.sbx.json'].as_string())
        e = next(x for x in side['shapes'] if x.get('new'))
        good = np.allclose(rec.get('diffuse'), (0.1, 0.2, 0.3, 1.0)) and rec.textures[0][1] == names.index(want) \
            and e['draws'][0]['textures'][0]['name'] == want
        report('new draw edited in the sidecar (diffuse, texture by name)', good, 'texture %s -> index %d' % (
            want, rec.textures[0][1]))

    print('PASS' if ok else 'FAIL')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
