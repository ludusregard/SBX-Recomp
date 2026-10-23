"""PART import: game files -> Blender objects.

    import_parts(context, paths, xpr_path=None)
    import_car(context, node_path, car_set=None, stock=False, wheels=())

A part becomes an empty (the part root) with one mesh object per shape
and one empty per marker. Each mesh keeps one Blender vertex per game
vertex, in order (no merging), with the normals as custom normals, one
material slot per draw, UV maps and the vertex colors. Tire morph
targets become shape keys of base_Shape. The part's sidecar goes into
the .blend as a text block, and the objects and materials are tagged
(properties.py) so the export can find their records.

A car block becomes its node file's hierarchy (empties), each part under
the node it attaches to, kept in place, and optionally wheels at the four
tire nodes, mirrored on the right.

The sidecar's two halves go into text blocks (properties.py); the
readable half also names each shape's Blender object and each draw's
Blender material.
"""

import os

import bpy
import numpy as np
from mathutils import Matrix

from . import frames, properties
from .log import get_logger
from .materials import MaterialMaker, TextureCache
from .sbx_formats import cars, read_NODE_nodes, read_PART, shaders, sidecar
from .sbx_formats.decode import decode_PART, morph_groups

log = get_logger(__name__)


def _matrix(m):
    return Matrix([list(map(float, row)) for row in m])


def _unit(v):
    n = np.linalg.norm(v, axis=1, keepdims=True)
    return np.where(n > 1e-8, v / np.where(n > 1e-8, n, 1), [0.0, 0.0, 1.0])


def _new_collection(context, name):
    coll = bpy.data.collections.new(name)
    context.scene.collection.children.link(coll)
    return coll


def _empty(coll, name, parent=None, local=None, display='PLAIN_AXES', size=0.1):
    obj = bpy.data.objects.new(name, None)
    obj.empty_display_type = display
    obj.empty_display_size = size
    coll.objects.link(obj)
    if parent is not None:
        obj.parent = parent
    if local is not None:
        obj.matrix_basis = _matrix(local)
    return obj


class Importer:
    """Shared state for one import: textures and materials made once."""

    def __init__(self, context):
        self.context = context
        self.textures = TextureCache()
        self.materials = MaterialMaker(self.textures)
        self.counts = {'parts': 0, 'shapes': 0, 'triangles': 0, 'markers': 0}

    # ------------------------------------------------------------ part

    def part(self, path, coll, parent=None, local=None, xpr_path=None, group='', stock=False):
        """Import one part file under parent; returns its root empty."""
        name = os.path.splitext(os.path.basename(path))[0]
        with open(path, 'rb') as f:
            data = f.read()
        pf = read_PART(data)
        dp = decode_PART(pf)
        log.info('read %s: %d shapes, %d nodes, %d shader pairs, XPR2 %s',
                 name, len(pf.shapes), len(pf.nodes), pf.shader_count,
                 os.path.basename(xpr_path) if xpr_path else 'none')

        sc = sidecar.to_sidecar(pf, name=name, source=data, texture_names=self.textures.names(xpr_path))

        _, pairs, _ = shaders.split_section(pf.shaders_raw)
        pair_hash = [p.vs_hash + ':' + p.ps_hash for p in pairs]

        root_name = '%s_%s' % (name, group) if group else name
        root = _empty(coll, root_name, parent, local, 'CUBE', 0.05)
        properties.tag_part_root(root, name, group, dp.attach or '')

        morphs = morph_groups(dp.shapes)
        targets = {t for ts in morphs.values() for t in ts}
        for i, shape in enumerate(dp.shapes):
            entry = sc['shapes'][i]
            if i in targets:
                entry['object'] = 'shape key of %s' % dp.shapes[next(b for b, ts in morphs.items() if i in ts)].name
                continue
            if stock and cars.STOCK_SKIP_SHAPE.match(shape.name):
                log.debug('%s %s: skipped (not the stock version)', name, shape.name)
                entry['object'] = 'not imported (not the stock version)'
                continue
            if shape.skinned:
                log.warning('%s %s: skinned; imported in its bind pose without bones', name, shape.name)
            for st in pf.shapes[i].unknown_streams():
                log.info('%s %s: stream type 0x%02X (stride %d) not fully worked out: kept in the sidecar; '
                         'the shape can\'t be edited', name, shape.name, st.type, st.stride)
            mats = []
            for k, rec in enumerate(pf.shapes[i].draws):
                sh = rec.get('shader')
                mats.append(self.materials.get(rec, xpr_path, properties.record_key(name, i, k),
                                               pair_hash[sh] if sh < len(pair_hash) else None))
            obj = self._mesh(coll, shape, mats, [dp.shapes[t] for t in morphs.get(i, ())])
            obj.parent = root
            properties.tag_shape(obj, name, i, shape.name)
            obj['sbx_normals'] = properties.normals_fingerprint(obj.data)
            entry['object'] = obj.name
            for d, m in zip(entry['draws'], mats):
                d['material'] = m.name
            self.counts['shapes'] += 1

        readable, raw = sidecar.split(sc)
        for text_name, body in ((properties.sidecar_text_name(name), sidecar.readable_text(readable)),
                                (properties.sidecar_raw_text_name(name), sidecar.raw_blob_text(raw))):
            text = bpy.data.texts.get(text_name) or bpy.data.texts.new(text_name)
            text.clear()
            text.write(body)

        for mk in dp.markers:
            if mk.position is None:
                continue
            m = np.eye(4)
            m[:3, 3] = np.asarray(mk.position) @ frames.NODE_TO_BLENDER
            e = _empty(coll, mk.name, root, m, 'SPHERE', 0.03)
            properties.tag_marker(e, name, mk.name)
            self.counts['markers'] += 1
        self.counts['parts'] += 1
        return root

    def _mesh(self, coll, shape, mats, targets=()):
        pos = (shape.positions @ frames.CAR_TO_BLENDER).astype(np.float32)
        tris, mat_index = [], []
        for k, draw in enumerate(shape.draws):
            if len(draw.triangles):
                tris.append(frames.flip_winding(draw.triangles.astype(np.int64)))
                mat_index.append(np.full(len(draw.triangles), k, np.int32))
        tris = np.concatenate(tris) if tris else np.zeros((0, 3), np.int64)
        mat_index = np.concatenate(mat_index) if mat_index else np.zeros(0, np.int32)

        me = bpy.data.meshes.new(shape.name)
        me.vertices.add(len(pos))
        me.vertices.foreach_set('co', pos.ravel())
        me.loops.add(tris.size)
        me.loops.foreach_set('vertex_index', tris.ravel().astype(np.int32))
        me.polygons.add(len(tris))
        me.polygons.foreach_set('loop_start', np.arange(0, tris.size, 3, dtype=np.int32))
        me.polygons.foreach_set('material_index', mat_index)
        # Shapes without stored normals (lights, LEDs, needles) shade flat,
        # as glTF does.
        me.polygons.foreach_set('use_smooth', np.full(len(tris), shape.normals is not None, bool))
        me.update(calc_edges=True)
        for m in mats:
            me.materials.append(m)

        corners = tris.ravel()
        # A third UV map (stream type 0x10) only when it isn't a copy of the
        # first, which it is in every shape of the game's parts.
        uvs3 = shape.uvs3
        if uvs3 is not None and shape.uvs is not None and np.array_equal(uvs3, shape.uvs):
            uvs3 = None
        for uvs, uv_name in ((shape.uvs, 'UVMap'), (shape.uvs2, 'UVMap.001'), (uvs3, 'UVMap.002')):
            if uvs is not None:
                layer = me.uv_layers.new(name=uv_name)
                uv = uvs[corners].astype(np.float32).copy()
                uv[:, 1] = 1.0 - uv[:, 1]   # Direct3D v = 0 at the top
                layer.data.foreach_set('uv', uv.ravel())
        if shape.colors is not None and (shape.colors != 255).any():
            attr = me.color_attributes.new('Col', 'BYTE_COLOR', 'POINT')
            rgba = shape.colors[:, [1, 2, 3, 0]].astype(np.float32) / 255.0   # stored ARGB
            attr.data.foreach_set('color_srgb', rgba.ravel())
        if shape.normals is not None:
            n = _unit(shape.normals @ frames.CAR_TO_BLENDER)
            me.normals_split_custom_set_from_vertices([tuple(v) for v in n])

        obj = bpy.data.objects.new(shape.name, me)
        coll.objects.link(obj)
        if targets:
            obj.shape_key_add(name='Basis', from_mix=False)
            for t in targets:
                key = obj.shape_key_add(name=t.name, from_mix=False)
                key.data.foreach_set('co', (t.positions @ frames.CAR_TO_BLENDER).astype(np.float32).ravel())
                key.value = 0.0   # new keys start at 1 in Blender 5.2
        self.counts['triangles'] += len(tris)
        return obj

    # ------------------------------------------------------------ car

    def car(self, node_path, car_set=None, stock=False, wheels=()):
        build_out = os.path.dirname(node_path)
        node_name = os.path.splitext(os.path.basename(node_path))[0]
        node_id = int(node_name)
        if not car_set:
            block = cars.car_block_of(node_id)
            if block is None or block[1] != node_id:
                raise ValueError('%s is not a car block node file; pick the set' % node_name)
            car_set = block[0]
        with open(node_path, 'rb') as f:
            nodes = read_NODE_nodes(f.read())
        log.info('car %s set %s: %d nodes, stock %s, wheels %s', node_name, car_set, len(nodes), stock, list(wheels))

        named = cars.car_name(node_id)
        label = '%s_%s' % (named[0], car_set) if named else 'car_%d_%s' % (node_id, car_set)
        coll = _new_collection(self.context, label)
        root = _empty(coll, label, display='ARROWS', size=0.5)
        if named:
            root['sbx_car'] = named[1]
        properties.tag_car_root(root, os.path.basename(node_path), car_set)
        objs, world = [], []
        for i, n in enumerate(nodes):   # parents come before their children
            local = frames.node_local(n)
            p = n.parent if 0 <= n.parent < i else None
            world.append((world[p] if p is not None else np.eye(4)) @ local)
            e = _empty(coll, n.name, objs[p] if p is not None else root, local, 'PLAIN_AXES', 0.05)
            properties.tag_node(e, os.path.basename(node_path), i)
            objs.append(e)

        body_xpr = cars.body_xpr_path(build_out, node_id)
        by_name = {n.name: i for i, n in enumerate(nodes)}
        for pid, path, group in cars.car_part_ids(build_out, node_id, car_set, stock):
            with open(path, 'rb') as f:
                attach = decode_PART(read_PART(f.read())).attach
            at = by_name.get(attach) if attach else None
            parent, local = root, None
            if at is not None:
                # Parts are modeled in place: cancel the attach node's
                # transform so the part stays where it is.
                parent, local = objs[at], np.linalg.inv(world[at])
            self.part(path, coll, parent, local, body_xpr, group, stock)

        tires = [by_name.get(t) for t in cars.TIRE_NODES]
        if wheels and None in tires:
            log.warning('no tire nodes in %s: wheels left out', node_name)
            wheels = ()
        for wid in wheels:
            path = os.path.join(build_out, '%08d.part' % wid)
            if not os.path.exists(path):
                log.warning('wheel part %s not found', os.path.basename(path))
                continue
            xpr = cars.wheel_xpr_path(build_out, wid)
            for tire, t in zip(cars.TIRE_NODES, tires):
                m = np.eye(4)
                if tire.endswith('r'):
                    m[0, 0] = -1.0   # wheel parts face +z in car space (+x here): mirror the right side
                self.part(path, coll, objs[t], m, xpr, tire)
        return root


def part_xpr(path):
    """The XPR2 a part's texture indices point into, found from its id:
    its car block's body XPR2, or for a wheel part the one or two ids
    before it. None if not found."""
    name = os.path.splitext(os.path.basename(path))[0]
    if not name.isdigit():
        return None
    folder = os.path.dirname(path)
    block = cars.car_block_of(int(name))
    if block is not None:
        p = cars.body_xpr_path(folder, block[1])
        if os.path.exists(p):
            return p
    return cars.wheel_xpr_path(folder, int(name))


def parse_wheels(text):
    """'587 588 581', 'SKY35' or 'AD03 583' -> part ids.

    A wheel's name (cars.WHEELS) stands for its rim and brake; numbers
    are part ids. Without a tire (cars.TIRES) the first tire is added."""
    ids = []
    for token in text.replace(',', ' ').split():
        if token.isdigit():
            ids.append(int(token))
            continue
        parts = cars.wheel_parts(token)
        if parts is None:
            raise ValueError('%s isn\'t a wheel name or a part id' % token)
        ids.extend(parts)
    if ids and not any(i in cars.TIRES for i in ids):
        ids.append(min(cars.TIRES))
    return ids


def import_parts(context, paths, xpr_path=None):
    imp = Importer(context)
    coll = _new_collection(context, 'SBX parts')
    for p in paths:
        imp.part(p, coll, xpr_path=xpr_path or part_xpr(p))
    log.info('imported %(parts)d parts: %(shapes)d shapes, %(triangles)d triangles, %(markers)d markers', imp.counts)
    return imp.counts


def import_car(context, node_path, car_set=None, stock=False, wheels=()):
    imp = Importer(context)
    imp.car(node_path, car_set, stock, wheels)
    log.info('imported car: %(parts)d parts, %(shapes)d shapes, %(triangles)d triangles, %(markers)d markers',
             imp.counts)
    return imp.counts
