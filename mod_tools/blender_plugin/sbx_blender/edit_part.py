"""Changing what a part holds, from Blender (the SBX panel's buttons).

    add_to_part(objects, target)       meshes become new shapes, empties
                                       new markers, of target's part
    remove_from_part(objects)          undo add_to_part
    make_material(mat, pair, template, textures, root)
                                       a Blender material becomes an SBX
                                       material with that shader pair
    sync_sidecar(part_name)            the readable sidecar says what the
                                       part now holds
    parent_needles(shape_objs, markers)  meter needles under their pivot

The part file itself is only made by the export; these tag objects and
materials (properties.py) and keep the readable sidecar up to date, so
it always shows what the export will write: each shape's Blender object
and materials, and under new_shapes, new_markers and
shader_pairs_brought_in what was added. The raw half isn't touched: it
stays the game's part.
"""

import json
import re

import bpy

from . import part_tree, properties
from .log import get_logger
from .sources import SourceError, Sources
from .sbx_formats import shaders, sidecar

log = get_logger(__name__)


class EditError(Exception):
    pass


def roots_of_part(part_name):
    return [o for o in bpy.data.objects if part_tree.is_root(o) and o['sbx_part'] == part_name]


def _check_name(name, what):
    try:
        b = name.encode('ascii')
    except UnicodeEncodeError:
        raise EditError('%s %r: names must be plain ASCII' % (what, name))
    if not b or len(b) > 31:
        raise EditError('%s %r: names are 1 to 31 characters' % (what, name))


def _taken_names(part_name, sources):
    """Shape and node names the part has, and the new ones joined to it."""
    sc = sources.sidecar(part_name)
    shapes = {s['name'] for s in sc['shapes']}
    nodes = {n['name'] for n in sc['nodes']}
    for root in roots_of_part(part_name):
        for c in part_tree.members(root):
            r = properties.new_shape_ref(c)
            if r:
                shapes.add(r[1])
            r = properties.new_marker_ref(c)
            if r:
                nodes.add(r[1])
    return shapes, nodes


def add_to_part(objects, target, sources=None):
    """Join objects to target's part: meshes as new shapes, empties as new
    markers. Each is parented to the part root where it is (or stays
    under a marker of the part). Returns [(object name, what it became)]."""
    sources = sources or Sources()
    root = part_tree.root_of(target)
    if root is None:
        raise EditError('%s isn\'t in an SBX part (the active object picks the part)' % target.name)
    part_name = root['sbx_part']
    shapes, nodes = _taken_names(part_name, sources)
    done = []
    members = part_tree.members(root)
    for obj in objects:
        if obj is root or part_tree.is_root(obj):
            continue
        if part_tree.root_of(obj) is root and not _is_copy(obj, members):
            if properties.shape_ref(obj) or obj.get('sbx_marker'):
                continue   # already the part's own
        if obj.type == 'MESH':
            name = properties.game_name(obj.name)
            if properties.new_shape_ref(obj) and properties.new_shape_ref(obj)[0] == part_name:
                continue
            _check_name(name, 'shape')
            if name in shapes:
                raise EditError('%s: the part already has a shape named %s; rename the object' % (obj.name, name))
            shapes.add(name)
            if part_tree.root_of(obj) is not root:
                part_tree.parent_keep_transform(obj, root)
            properties.tag_new_shape(obj, part_name, name)
            done.append((obj.name, 'new shape %s' % name))
        elif obj.type == 'EMPTY':
            name = properties.game_name(obj.name)
            _check_name(name, 'marker')
            if name in nodes:
                raise EditError('%s: the part already has a node named %s; rename the empty' % (obj.name, name))
            nodes.add(name)
            if part_tree.root_of(obj) is not root:
                part_tree.parent_keep_transform(obj, root)
            properties.tag_new_marker(obj, part_name, name)
            done.append((obj.name, 'new marker %s' % name))
    for name, what in done:
        log.info('%s: %s joined as %s', part_name, name, what)
    sync_sidecar(part_name, sources)
    return done


def _is_copy(obj, members):
    """A copy of one of the part's shapes or markers (Shift+D copies the
    tags): its name isn't the one it claims, or another object claims the
    same shape or marker."""
    ref = properties.shape_ref(obj)
    if ref:
        if properties.game_name(obj.name) != ref[2]:
            return True
        return any(o is not obj and properties.shape_ref(o) == ref for o in members)
    mk = obj.get('sbx_marker')
    if mk:
        if properties.game_name(obj.name) != mk:
            return True
        return any(o is not obj and o.get('sbx_marker') == mk for o in members)
    return False


def remove_from_part(objects):
    """New shapes and markers stop being part of their part (they stay where they are)."""
    parts = set()
    for obj in objects:
        r = properties.new_shape_ref(obj) or properties.new_marker_ref(obj)
        if r:
            parts.add(r[0])
            properties.untag_new(obj)
            log.info('%s: %s removed from the part', r[0], obj.name)
    for p in parts:
        sync_sidecar(p)
    return len(parts)


def make_material(mat, pair_key, template_ref, textures, root=None, sources=None):
    """Make mat an SBX material: shader pair pair_key, starting from the
    record template_ref ('part:shape:draw' or 'library:...'), binding
    textures [(xpr, index)] (as many as the template binds; empty: the
    template's)."""
    sources = sources or Sources()
    rec, key = sources.record(template_ref)
    if key != pair_key:
        raise EditError('template %s uses another shader pair' % template_ref)
    bound = rec.textures
    if not textures:
        textures = bound
    if len(textures) != len(bound):
        raise EditError('the template binds %d textures, %d given' % (len(bound), len(textures)))
    names = properties.xpr_names(root)
    tex_names = [names[t] if 0 <= t < len(names) else 'tex%d' % t for _, t in textures]
    properties.tag_new_material(mat, template_ref, pair_key, rec, textures, tex_names,
                                root.get('sbx_xpr', '') if root is not None else '')
    log.info('material %s: SBX material, shader %s, from %s, textures %s', mat.name, pair_ids(pair_key),
             template_ref, ', '.join(tex_names) or 'none')
    for p in {properties.shape_ref(o)[0] for o in bpy.data.objects if properties.shape_ref(o)
              and any(s.material is mat for s in o.material_slots)} | \
            {properties.new_shape_ref(o)[0] for o in bpy.data.objects if properties.new_shape_ref(o)
             and any(s.material is mat for s in o.material_slots)}:
        sync_sidecar(p, sources)


def pair_ids(key):
    """'vs_hash:ps_hash' -> 'vs_02 + ps_03'."""
    vs, _, ps = key.partition(':')
    return '%s + %s' % (shaders.shader_id(vs, 'vs'), shaders.shader_id(ps, 'ps'))


def sync_sidecar(part_name, sources=None):
    """Bring the part's readable sidecar up to date with the scene: the part
    is built as the export would build it, and the readable half is
    written from that (write_readable). If it can't be built, the reason
    goes into the sidecar as export_problem and the rest stays as it was."""
    from . import export_part
    text = bpy.data.texts.get(properties.sidecar_text_name(part_name))
    if text is None:
        return None
    roots = roots_of_part(part_name)
    if not roots:
        return None
    try:
        with part_tree.object_mode():
            res = export_part.build_part(part_name, roots, sources=sources or Sources())
    except (export_part.ExportError, SourceError, EditError) as e:
        try:
            readable = json.loads(text.as_string())
        except ValueError:
            return str(e)
        readable['export_problem'] = str(e)
        text.clear()
        text.write(sidecar.readable_text(readable))
        log.warning('%s: sidecar not refreshed: %s', part_name, e)
        return str(e)
    write_readable(part_name, res)
    return None


def _stream_words(shape):
    from .sbx_formats import streams as streams_mod
    return ['slot %d: type 0x%02X, stride %d: %s' % (k, st.type, st.stride, streams_mod.describe(st.type, st.stride))
            for k, st in enumerate(shape.streams) if st is not None]


def write_readable(part_name, res):
    """The readable sidecar from a built part (export_part.BuildResult).

    Same layout as the import writes, for everything the part will hold:
    the game's shapes in their order, then the new ones ("new": true),
    each with its streams and every draw in full (textures with index
    and name, shader, colors, flags, fields not worked out). A draw whose
    record comes from a material says so (record_from); a draw in a slot
    the game's shape didn't have, and every draw of a new shape, has its
    slot number. Markers, nodes and shader pairs are listed as in the
    written part, the added ones marked new. Every draw's values are
    editable here; the export writes them (for a draw from a material,
    while the entry names the slot's current material)."""
    text = bpy.data.texts.get(properties.sidecar_text_name(part_name))
    if text is None:
        return
    try:
        old = json.loads(text.as_string())
    except ValueError:
        old = None
    sc = res.build.readable
    part = res.part
    names = res.build.names or None
    _, pairs, _ = shaders.split_section(part.shaders_raw)
    raw_pairs = shaders.split_section(sidecar.unb64(sc['raw']['shaders']))[1]
    old_keys = [p.vs_hash + ':' + p.ps_hash for p in raw_pairs]
    by_name = {s.name: (k, s) for k, s in enumerate(part.shapes)}
    readable = {k: v for k, v in (old or sc).items() if k not in ('raw', 'new_shapes', 'new_markers',
                                                                   'shader_pairs_brought_in', 'export_problem')}
    readable['part'] = sc['part']

    def draws_of(shape, info, n_game):
        out = []
        for (slot, mat, ref), rec in zip(info.draws, shape.draws):
            body = sidecar._readable_draw(rec, slot, pairs, names)
            d = {'draw': slot, 'slot': slot + 1}
            if info.new or slot >= n_game:
                d['new'] = True
            d['material'] = mat
            if ref != 'game':
                d['record_from'] = ref
            d.update((k, v) for k, v in body.items() if k not in ('draw', 'material'))
            out.append(d)
        return out

    shapes_out = []
    game = [e for e in sc['shapes'] if not e.get('new')]
    for i, e in enumerate(game):
        e = dict(e)
        info = res.build.slots.get(e['name'])
        hit = by_name.get(e['name'])
        if info is not None and hit is not None:
            k, shape = hit
            e['object'] = info.object
            e['vertices'] = next((st.count for st in shape.streams if st is not None), 0)
            e['streams'] = _stream_words(shape)
            e['draws'] = draws_of(shape, info, len(sc['raw']['shapes'][i]['draws']))
        e.pop('new_slots', None)
        shapes_out.append(e)
    for name, info in res.build.slots.items():
        if not info.new or name not in by_name:
            continue
        k, shape = by_name[name]
        shapes_out.append({
            'shape': k, 'name': name, 'new': True, 'object': info.object,
            'vertices': next((st.count for st in shape.streams if st is not None), 0),
            'layout_from': info.template, 'streams': _stream_words(shape),
            'skinned': False, 'draws': draws_of(shape, info, 0)})
    readable['shapes'] = shapes_out

    game_nodes = {}
    for n in sc['nodes']:
        game_nodes[n['name']] = game_nodes.get(n['name'], 0) + 1
    seen, nodes, markers = {}, [], []
    emb = part.embedded_nodes.nodes if part.embedded_nodes else []
    for n in part.nodes:
        seen[n.name] = seen.get(n.name, 0) + 1
        e = {'name': n.name, 'kind': 'shape' if n.is_shape_node else 'marker', 'embedded_node': n.get('node_index')}
        if n.is_shape_node:
            e['shape'] = n.get('shape_index')
            e['shape_name'] = part.shapes[n.get('shape_index')].name
        k = part.nodes.index(n)
        if 0 <= k < len(emb):
            e['position_node_frame'] = [round(float(x), 6) for x in emb[k].translation]
        if seen[n.name] > game_nodes.get(n.name, 0):
            e['new'] = True
        nodes.append(e)
        if not n.is_shape_node:
            m = {'name': n.name, 'position_node_frame': e.get('position_node_frame')}
            if e.get('new'):
                m['new'] = True
            markers.append(m)
    readable['markers'] = markers
    readable['nodes'] = nodes
    readable['shader_pairs'] = [dict({'slot': k, 'shader': p.ids, 'vs_hash': p.vs_hash, 'ps_hash': p.ps_hash},
                                     **({'new': True} if p.vs_hash + ':' + p.ps_hash not in old_keys else {}))
                                for k, p in enumerate(pairs)]
    order = ['format', 'version', 'part', 'shapes', 'markers', 'nodes', 'shader_pairs']
    readable = {k: readable[k] for k in order if k in readable} | \
        {k: v for k, v in readable.items() if k not in order}
    text.clear()
    text.write(sidecar.readable_text(readable))


# ---------------------------------------------------------------- needles

NEEDLE = re.compile(r'^(.*?)hari(\d*)_Shape$')


def needle_marker(shape_name, marker_names):
    """The marker a meter needle turns about, by name, or None:
    sm_hari00_Shape -> sm_hari, dm_hari01_Shape -> dm_01."""
    m = NEEDLE.match(shape_name)
    if not m:
        return None
    pre, nn = m.groups()
    for cand in (pre + 'hari' + nn, pre + 'hari', pre + nn):
        if cand in marker_names:
            return cand
    return None


def parent_needles(shape_objs, markers):
    """Parent each needle shape to its marker without moving it.
    shape_objs: {shape name: object}; markers: {marker name: empty}."""
    out = []
    for name, obj in shape_objs.items():
        mk = needle_marker(name, markers)
        if mk is None:
            continue
        marker = markers[mk]
        obj.parent = marker
        obj.matrix_parent_inverse = marker.matrix_basis.inverted()
        out.append((name, mk))
    return out
