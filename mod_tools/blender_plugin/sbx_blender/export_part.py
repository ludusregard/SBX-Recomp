"""PART export: Blender objects -> part files.

    export_parts(context, folder, skip_identical=False, apply_modifiers=False)

Which parts: every part root (an empty tagged sbx_part, made by the
import) with at least one visible shape object, new or not. A part goes
out whole: its hidden shapes are written too, since the game finds
shapes by name.

How each part is built:
1. Its sidecar is loaded and the edits made in the readable half are
   applied to the records (sidecar.apply_draw_entry): colors, power,
   params, flags, blend, shared shader, texture indices, shader by id.
2. Each shape is compared with the original (vertex positions within
   1e-5 m, the same triangles in the same order and material slots, the
   same UVs and vertex colors, the object's own transform, the normals'
   fingerprint taken at import, the tire morphs' shape keys). A shape that
   matches is written from the original bytes, so an untouched part
   comes out identical to the game's file.
3. An edited shape is encoded from its mesh (sbx_formats.encode):
   polygons are triangulated, a vertex is split where its corners
   disagree on UV, normal or color, normals are the mesh's corner
   normals. Each material slot is a draw: slot k keeps the original draw
   k's record unless another material was put in it; then (and for a new
   slot) the material's record is used: an imported material's own
   record, or a new material's template record with its values
   (sources.py). Slots with no faces left are dropped.
4. New shapes (meshes joined with Add to Part) are encoded the same way,
   in the stream layout of a shape their materials came from or of one
   of the part's own (encode.choose_template), and added at their
   sorted place with their node and embedded node (sbx_formats.compose).
5. New markers (empties joined with Add to Part) are added to the node
   list and the embedded node file at their position.
A material whose shader pair the part doesn't have brings the pair in:
it's appended to the part's shader section, from another imported part
or from the shader library.

Tire morphs: the tires' target shapes are shape keys of base_Shape. An
edited tire writes base_Shape from the mesh and each target from its
shape key (same name), all with the same vertex split.

Markers (empties tagged sbx_marker) that were moved are written into
the part's embedded node file (position only; a rotation is ignored,
with a warning). A shape under a marker (the meter needles) is written
where it sits with the marker at rest (part_tree.to_root).

Not possible yet (the export stops and says why): editing the two
shapes with stream type 0x0D (one bonnet model), shape keys on shapes
other than the tire morphs, skinned shapes, more than 65,535 vertices,
a material that isn't an SBX material (imported, or made with Make SBX
Material), a shader pair that's in no imported part and not in the
shader library.

apply_modifiers uses the evaluated mesh (modifiers and all); otherwise
modifiers are ignored. Files are named <part>.part (the patch loader
ignores the extension; docs/game_data.md, "Patch data"). A part imported
more than once (the wheels, at each tire) is written once; if its
copies differ, the export stops.
"""

import collections
import os

import numpy as np

from . import frames, part_tree, properties
from .log import get_logger
from .materials import material_key
from .sources import SourceError, Sources  # noqa: F401 (SourceError: the operator catches it here)
from .sbx_formats import read_PART, shaders, sidecar
from .sbx_formats.compose import ComposeError, add_marker, add_shader_pair, add_shape, check_rules
from .sbx_formats.decode import decode_PART, morph_groups
from .sbx_formats.encode import EncodeError, choose_template, encode_new_shape, encode_shape
from .sbx_formats.node import read_NODE, set_node_translation
from .sbx_formats.part_write import write_PART

log = get_logger(__name__)

POSITION_TOLERANCE = 1e-5   # meters
MARKER_TOLERANCE = 1e-4     # node frame units (decimeters)
UV_TOLERANCE = 1e-5
NORMAL_MERGE_DEGREES = 1.0


class ExportError(Exception):
    pass


def part_roots(scene):
    """{part name: [root objects]} for the parts with a visible shape."""
    roots = {}
    for obj in scene.objects:
        if not part_tree.is_root(obj):
            continue
        shapes = [c for c in part_tree.members(obj)
                  if properties.shape_ref(c) or properties.new_shape_ref(c)]
        if any(c.visible_get() for c in shapes):
            roots.setdefault(obj['sbx_part'], []).append(obj)
    return roots


# ---------------------------------------------------------------- mesh

class MeshData:
    """A shape object's mesh in part (car) space, per vertex and per corner."""

    def __init__(self, obj, me, local):
        self.obj = obj
        self.fingerprint = properties.normals_fingerprint(me)
        n = len(me.vertices)
        co = np.zeros(n * 3, np.float32)
        me.vertices.foreach_get('co', co)
        co = co.reshape(n, 3).astype(np.float64)
        self.moved = not np.allclose(local, np.eye(4))
        if self.moved:
            co = co @ local[:3, :3].T + local[:3, 3]
        self.positions = co @ frames.CAR_TO_BLENDER.T   # orthonormal: inverse = transpose
        self.normal_matrix = np.linalg.inv(local[:3, :3]).T if self.moved else np.eye(3)
        self.sizes = np.zeros(len(me.polygons), np.int32)
        me.polygons.foreach_get('loop_total', self.sizes)
        self.loops = np.zeros(len(me.loops), np.int32)
        me.loops.foreach_get('vertex_index', self.loops)
        self.slots = np.zeros(len(me.polygons), np.int32)
        me.polygons.foreach_get('material_index', self.slots)
        self.uvs = []
        for layer in me.uv_layers[:3]:
            d = np.zeros(len(me.loops) * 2, np.float32)
            layer.data.foreach_get('uv', d)
            self.uvs.append(d.reshape(-1, 2))
        self.colors = None          # per vertex or per corner, ARGB bytes
        self.color_domain = None
        attr = me.color_attributes.get('Col')
        if attr is not None and attr.domain in ('POINT', 'CORNER'):
            d = np.zeros(len(attr.data) * 4, np.float32)
            attr.data.foreach_get('color_srgb', d)
            rgba = np.clip(np.round(d.reshape(-1, 4) * 255.0), 0, 255).astype(np.uint8)
            self.colors = rgba[:, [3, 0, 1, 2]]
            self.color_domain = attr.domain
        me.calc_loop_triangles()
        nt = len(me.loop_triangles)
        self.tri_loops = np.zeros(nt * 3, np.int32)
        me.loop_triangles.foreach_get('loops', self.tri_loops)
        self.tri_loops = self.tri_loops.reshape(nt, 3)
        self.tri_slots = np.zeros(nt, np.int32)
        me.loop_triangles.foreach_get('material_index', self.tri_slots)
        nrm = np.zeros(len(me.loops) * 3, np.float32)
        me.corner_normals.foreach_get('vector', nrm)
        nrm = nrm.reshape(-1, 3).astype(np.float64) @ self.normal_matrix.T @ frames.CAR_TO_BLENDER.T
        self.corner_normals = nrm / np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-12)
        self.local = local
        self.keys = {}   # shape key name -> (positions in car space, corner normals in car space)
        sk = me.shape_keys
        if sk is not None:
            for kb in sk.key_blocks[1:]:
                kc = np.zeros(n * 3, np.float32)
                kb.data.foreach_get('co', kc)
                kc = kc.reshape(n, 3).astype(np.float64)
                if self.moved:
                    kc = kc @ local[:3, :3].T + local[:3, 3]
                kn = np.array(kb.normals_split_get(), np.float64).reshape(-1, 3)
                kn = kn @ self.normal_matrix.T @ frames.CAR_TO_BLENDER.T
                kn /= np.maximum(np.linalg.norm(kn, axis=1, keepdims=True), 1e-12)
                self.keys[kb.name] = (kc @ frames.CAR_TO_BLENDER.T, kn)

    def matches(self, shape):
        """None if this still is the decoded original shape, otherwise why not."""
        if self.moved:
            return 'object moved'
        imported = self.obj.get('sbx_normals')
        if imported is not None and imported != self.fingerprint and len(self.positions) == len(shape.positions) \
                and np.abs(self.positions - shape.positions).max(initial=0.0) <= POSITION_TOLERANCE:
            return 'normals changed'
        if len(self.positions) != len(shape.positions):
            return 'vertex count %d, was %d' % (len(self.positions), len(shape.positions))
        if np.abs(self.positions - shape.positions).max(initial=0.0) > POSITION_TOLERANCE:
            return 'vertices moved'
        if (self.sizes != 3).any():
            return 'faces that aren\'t triangles'
        tris, want_slots = [], []
        for k, draw in enumerate(shape.draws):
            tris.append(frames.flip_winding(draw.triangles.astype(np.int64)))
            want_slots.append(np.full(len(draw.triangles), k))
        tris = np.concatenate(tris) if tris else np.zeros((0, 3), np.int64)
        want_slots = np.concatenate(want_slots) if want_slots else np.zeros(0, np.int64)
        if len(self.loops) != tris.size or not np.array_equal(self.loops, tris.ravel()):
            return 'faces changed'
        if not np.array_equal(self.slots, want_slots):
            return 'material slots of faces changed'
        corners = tris.ravel()
        uvs3 = shape.uvs3 if shape.uvs3 is not None and not np.array_equal(shape.uvs3, shape.uvs) else None
        want_uvs = [u for u in (shape.uvs, shape.uvs2, uvs3) if u is not None]
        if len(self.uvs) != len(want_uvs):
            return 'UV maps added or removed'
        for got, want in zip(self.uvs, want_uvs):
            w = want[corners].astype(np.float64).copy()
            w[:, 1] = 1.0 - w[:, 1]
            if np.abs(got - w).max(initial=0.0) > UV_TOLERANCE:
                return 'UVs changed'
        if self.colors is not None and shape.colors is not None:
            got = self.colors if self.color_domain == 'POINT' else None
            if got is None or not np.array_equal(got, shape.colors):
                return 'vertex colors changed'
        return None

    def game_vertices(self):
        """Split vertices where corners disagree -> (positions, normals, uvs,
        uvs2, colors, triangles in the game's winding, triangle slots).

        Corners of a Blender vertex stay one game vertex when their UVs and
        colors are the same and their normals are within NORMAL_MERGE_DEGREES
        (Blender's corner normals differ by thousandths of a degree even on
        a smooth vertex); the vertex gets their average normal."""
        corners = self.tri_loops.ravel()
        verts = self.loops[corners]
        uv = [u[corners] for u in self.uvs]
        cols = [verts.astype(np.float64)[:, None]] + uv
        if self.colors is not None:
            c = self.colors[verts] if self.color_domain == 'POINT' else self.colors[corners]
            cols.append(c.astype(np.float64))
        _, group = np.unique(np.concatenate(cols, 1), axis=0, return_inverse=True)
        group = group.ravel()
        normals = self.corner_normals[corners]
        # Split a group only where its normals really differ.
        sub = np.zeros(len(corners), np.int64)
        order = np.argsort(group, kind='stable')
        starts = np.flatnonzero(np.r_[True, group[order][1:] != group[order][:-1]])
        ref = normals[order[starts]]
        ref_of = np.repeat(ref, np.diff(np.r_[starts, len(order)]), axis=0)
        far = np.empty(len(corners), bool)
        far[order] = (ref_of * normals[order]).sum(1) < np.cos(np.radians(NORMAL_MERGE_DEGREES))
        for g in np.unique(group[far]):
            members = np.flatnonzero(group == g)
            centers = []
            for m in members:
                for j, cn in enumerate(centers):
                    if np.dot(cn, normals[m]) >= np.cos(np.radians(NORMAL_MERGE_DEGREES)):
                        sub[m] = j
                        break
                else:
                    sub[m] = len(centers)
                    centers.append(normals[m])
        _, first, inverse = np.unique(np.stack([group, sub], 1), axis=0, return_index=True, return_inverse=True)
        inverse = inverse.ravel()
        # Keep Blender's vertex order: by vertex, then by first use.
        order = np.lexsort((first, verts[first]))
        rank = np.empty(len(order), np.int64)
        rank[order] = np.arange(len(order))
        new_index = rank[inverse]
        firsts = first[order]
        summed = np.zeros((len(first), 3))
        np.add.at(summed, new_index, normals)
        out_normals = summed / np.maximum(np.linalg.norm(summed, axis=1, keepdims=True), 1e-12)
        positions = self.positions[verts[firsts]]
        uvs = [u[firsts].astype(np.float64).copy() for u in uv]
        for u in uvs:
            u[:, 1] = 1.0 - u[:, 1]      # Direct3D v = 0 at the top
        colors = None
        if self.colors is not None:
            colors = (self.colors[verts[firsts]] if self.color_domain == 'POINT'
                      else self.colors[corners[firsts]])
        tris = frames.flip_winding(new_index.reshape(-1, 3))
        self._split = (verts, firsts, new_index, corners, len(first))
        self.uvs3 = uvs[2] if len(uvs) > 2 else None
        return (positions, out_normals, uvs[0] if uvs else None, uvs[1] if len(uvs) > 1 else None,
                colors, tris, self.tri_slots)

    def remap(self, positions, corner_normals):
        """Per-vertex positions and per-corner normals (of a shape key) ->
        game vertices, split as the last game_vertices call split them."""
        verts, firsts, new_index, corners, count = self._split
        summed = np.zeros((count, 3))
        np.add.at(summed, new_index, corner_normals[corners])
        normals = summed / np.maximum(np.linalg.norm(summed, axis=1, keepdims=True), 1e-12)
        return positions[verts[firsts]], normals


def _mesh_of(obj, root, apply_modifiers, depsgraph):
    local = part_tree.to_root(obj, root)
    if apply_modifiers and obj.modifiers:
        ev = obj.evaluated_get(depsgraph)
        me = ev.to_mesh()
        try:
            return MeshData(obj, me, local)
        finally:
            ev.to_mesh_clear()
    if obj.modifiers:
        log.warning('%s: modifiers ignored (Apply modifiers is off)', obj.name)
    return MeshData(obj, obj.data, local)


# ---------------------------------------------------------------- records

class PartBuild:
    """The part being written: its model, the readable sidecar its draw
    edits come from, the shader pairs it gains, and which record went into
    which draw (for the sidecar, edit_part.sync_sidecar)."""

    def __init__(self, name, part, sources, readable, names):
        self.name = name
        self.part = part
        self.sources = sources
        self.readable = readable
        self.names = names            # the XPR2's texture names, if known
        self.added_pairs = []
        self.slots = {}               # shape name -> SlotInfo
        self.changes = []

    def pairs(self):
        return shaders.split_section(self.part.shaders_raw)[1]

    def pair_keys(self):
        return [p.vs_hash + ':' + p.ps_hash for p in self.pairs()]

    def slot_for(self, key, why):
        """The part's shader slot for pair key, bringing the pair in if needed."""
        keys = self.pair_keys()
        if key in keys:
            return keys.index(key)
        try:
            pair = self.sources.pair(key)
        except SourceError as e:
            raise ExportError('%s %s: %s' % (self.name, why, e))
        k = add_shader_pair(self.part, pair)
        self.added_pairs.append(pair.ids)
        log.info('%s: shader pair %s brought in as slot %d (for %s)', self.name, pair.ids, k, why)
        return k

    def slot_for_ids(self, why):
        """shader ids ('vs_02 + ps_03') -> slot, for a shader edited in the sidecar."""
        def get(ids):
            key = shaders.pair_key(ids)
            if key is None:
                raise ExportError('%s %s: shader %r isn\'t a known pair (ids as in docs/game_data.md)'
                                  % (self.name, why, ids))
            return self.slot_for(key, why)
        return get

    def material(self, mat, why):
        """(record with this part's shader slot, template reference) for an SBX material."""
        try:
            rec, key, ref = self.sources.material_record(mat)
        except SourceError as e:
            raise ExportError('%s %s: %s' % (self.name, why, e))
        rec.set('shader', self.slot_for(key, why))
        return rec, ref

    def entry_edits(self, rec, entry, where):
        """Apply a readable draw entry's edits to a record taken from a material."""
        if entry is None:
            return
        sidecar.resolve_texture_names(entry, self.names)
        try:
            ch = sidecar.apply_draw_entry(rec, entry, self.pairs(), where, self.slot_for_ids(where))
        except sidecar.EditError as e:
            raise ExportError('%s: %s (in the sidecar)' % (self.name, e))
        self.changes += ch


class SlotInfo:
    """What went into a shape's draws: per written draw, its material slot
    (0-based), the slot's material name and where the record came from
    ('game' for the part's own record, else the material's reference)."""

    def __init__(self, obj, new=False, template=None):
        self.object = obj.name if obj is not None else None
        self.new = new
        self.template = template
        self.draws = []     # [(slot, material name, ref)]


def _shape_entry(readable, shape_index=None, new_name=None):
    for k, e in enumerate(readable.get('shapes', [])):
        if new_name is not None and e.get('new') and e.get('name') == new_name:
            return e
        if shape_index is not None and not e.get('new') and k == shape_index:
            return e
    return None


def _draw_entry(shape_entry, slot, mat_name):
    """The readable entry for a slot's draw, when it describes the slot's
    current material (an entry for another material is stale)."""
    if shape_entry is None:
        return None
    for d in shape_entry.get('draws', []):
        s = d.get('slot', d.get('draw', -1) + 1)
        if s == slot + 1 and d.get('material') == mat_name and not sidecar.is_game_draw(d):
            return d
    return None


def _slot_records(build, obj, i, original_records, original_keys):
    """[(record, ref)] for each of an existing shape's material slots."""
    out = []
    entry = _shape_entry(build.readable, shape_index=i)
    for k, slot in enumerate(obj.material_slots):
        mat = slot.material
        if k < len(original_records) and (mat is None or mat.get('sbx_key') == original_keys[k]):
            out.append((original_records[k], 'game'))
            continue
        where = '%s slot %d' % (obj.name, k + 1)
        rec, ref = build.material(mat, where)
        build.entry_edits(rec, _draw_entry(entry, k, mat.name), where)
        log.info('%s %s: slot %d takes the record of %s (%s)', build.name, obj.name, k + 1,
                 mat.name if mat else '-', ref)
        out.append((rec, ref))
    return out


def _edits_copy(sc):
    """A sidecar copy whose entries can be changed without touching the cached one."""
    import copy
    raw = dict(sc['raw'])
    raw['shapes'] = [dict(s, draws=[dict(d) for d in s['draws']]) for s in raw['shapes']]
    out = copy.deepcopy({k: v for k, v in sc.items() if k != 'raw'})
    out['raw'] = raw
    return out


def _game_draw_edits(build, part):
    """The readable edits of the game's own draws, onto the part's records."""
    for i, (entry, shape) in enumerate(zip(build.readable['shapes'], part.shapes)):
        if entry.get('new'):
            continue
        for d in entry.get('draws', []):
            if not sidecar.is_game_draw(d):
                continue
            k = d.get('draw', -1)
            if not 0 <= k < len(shape.draws):
                continue
            where = '%s draw %d' % (shape.name, k)
            sidecar.resolve_texture_names(d, build.names)
            try:
                build.changes += sidecar.apply_draw_entry(shape.draws[k], d, build.pairs(), where,
                                                          build.slot_for_ids(where))
            except sidecar.EditError as e:
                raise ExportError('%s: %s (in the sidecar)' % (build.name, e))


# ---------------------------------------------------------------- parts

class BuildResult:
    def __init__(self, data, sha, changes, part, build):
        self.data = data
        self.original_sha256 = sha
        self.changes = changes
        self.part = part            # the model that was written
        self.build = build


def build_part(part_name, roots, apply_modifiers=False, depsgraph=None, sources=None):
    """A part's file from its sidecar and its objects -> BuildResult."""
    sources = sources or Sources()
    try:
        sc = _edits_copy(sources.sidecar(part_name))
    except SourceError as e:
        raise ExportError(str(e))
    unedited = sidecar.from_sidecar(sc)
    part = sidecar.from_sidecar(sc)
    rules_before = check_rules(part)
    decoded = decode_PART(unedited)
    names = next((properties.xpr_names(r) for r in roots if properties.xpr_names(r)), [])
    build = PartBuild(part_name, part, sources, sc, names)
    pair_hash = build.pair_keys()
    _game_draw_edits(build, part)

    morphs = morph_groups(decoded.shapes)
    results = {}   # shape index -> (root name, new shapes or None)
    moved_markers = {}   # embedded node index -> position (node frame)
    new_shapes = collections.OrderedDict()    # name -> (root, obj)
    new_markers = collections.OrderedDict()   # name -> (root, obj)
    for root in roots:
        members = part_tree.members(root)
        _markers(part_name, root, members, part, moved_markers)
        by_index = {}
        for c in members:
            ref = properties.shape_ref(c)
            if ref and ref[0] == part_name:
                by_index.setdefault(ref[1], []).append(c)
            ref = properties.new_shape_ref(c)
            if ref and ref[0] == part_name:
                if ref[1] in new_shapes and new_shapes[ref[1]][1] is not c:
                    raise ExportError('%s: two new shapes named %s (%s, %s)'
                                      % (part_name, ref[1], new_shapes[ref[1]][1].name, c.name))
                new_shapes[ref[1]] = (root, c)
            ref = properties.new_marker_ref(c)
            if ref and ref[0] == part_name:
                new_markers.setdefault(ref[1], (root, c))
        for i, shape in enumerate(decoded.shapes):
            objs = by_index.get(i, [])
            if len(objs) > 1:
                raise ExportError('%s: %d objects claim shape %d (%s): %s. A copy (Shift+D) keeps the original\'s '
                                  'tags: rename it, select it and the part, and press Add to Part'
                                  % (part_name, len(objs), i, shape.name, ', '.join(o.name for o in objs)))
            if not objs:
                continue    # a tire morph target (a shape key) or a shape the stock import skipped
            obj = objs[0]
            targets = morphs.get(i, [])
            if obj.data.shape_keys is not None and not targets:
                raise ExportError('%s %s: has shape keys; only the tire morphs (base_Shape) can have them'
                                  % (part_name, obj.name))
            if apply_modifiers and obj.modifiers and targets:
                raise ExportError('%s %s: Apply modifiers can\'t keep the tire morphs (shape keys); '
                                  'apply the modifiers by hand first' % (part_name, obj.name))
            mesh = _mesh_of(obj, root, apply_modifiers, depsgraph)
            why = mesh.matches(shape)
            if why is None:
                for t in targets:
                    key = mesh.keys.get(decoded.shapes[t].name)
                    if key is None:
                        why = 'shape key %s missing' % decoded.shapes[t].name
                    elif np.abs(key[0] - decoded.shapes[t].positions).max(initial=0.0) > POSITION_TOLERANCE:
                        why = 'shape key %s changed' % decoded.shapes[t].name
            if why is None:
                why = _materials_changed(obj, unedited.shapes[i], pair_hash)
            new = {}
            if why is not None:
                new = _encode(build, obj, i, why, mesh, unedited, pair_hash,
                              [(t, decoded.shapes[t].name) for t in targets])
                build.changes.append('%s: %s' % (shape.name, why))
            else:
                info = build.slots[shape.name] = SlotInfo(obj)
                info.draws = [(k, obj.material_slots[k].material.name
                               if k < len(obj.material_slots) and obj.material_slots[k].material else None, 'game')
                              for k in range(len(part.shapes[i].draws))]
            if i in results and (not results[i][1]) != (not new):
                raise ExportError('%s: imported more than once and the copies differ (%s); edit one copy only '
                                  'and delete the others' % (part_name, shape.name))
            results[i] = (root.name, new)
    for i, (_, new) in results.items():
        for k, shape in new.items():
            part.shapes[k] = shape
    # Moved markers first: adding nodes below changes the indices.
    for index, pos in moved_markers.items():
        part.embedded_raw = set_node_translation(part.embedded_raw, index, pos)
        build.changes.append('marker %s moved' % part.embedded_nodes.nodes[index].name)
    if moved_markers:
        part.embedded_nodes = read_NODE(part.embedded_raw)
    for name, (root, obj) in new_shapes.items():
        shape = _encode_new(build, obj, root, name, unedited, apply_modifiers, depsgraph)
        try:
            add_shape(part, shape)
        except ComposeError as e:
            raise ExportError('%s %s: %s' % (part_name, obj.name, e))
        build.changes.append('new shape %s (%s)' % (name, obj.name))
    for name, (root, obj) in new_markers.items():
        pos = part_tree.to_root(obj, root)[:3, 3] @ np.linalg.inv(frames.NODE_TO_BLENDER)
        try:
            add_marker(part, name, pos)
        except ComposeError as e:
            raise ExportError('%s marker %s: %s' % (part_name, obj.name, e))
        build.changes.append('new marker %s' % name)
    for ids in build.added_pairs:
        build.changes.append('shader pair %s brought in' % ids)
    data = write_PART(part)
    back = read_PART(data)   # it must read back
    broken = [r for r in check_rules(back) if r not in rules_before]
    if broken:
        raise ExportError('%s: the written part breaks the game\'s rules: %s (a bug; see the log)'
                          % (part_name, '; '.join(broken)))
    return BuildResult(data, sidecar.source_sha256(sc), build.changes, back, build)


def _original_keys(obj, original, pair_hash):
    """The material key each of a shape's original draws had at import."""
    return [material_key(r, pair_hash[r.get('shader')] if r.get('shader') < len(pair_hash) else None,
                         obj.material_slots[k].material.get('sbx_xpr', '')
                         if k < len(obj.material_slots) and obj.material_slots[k].material else '')
            for k, r in enumerate(original.draws)]


def _materials_changed(obj, original, pair_hash):
    """Why a shape's materials aren't the ones it was imported with, or None."""
    keys = _original_keys(obj, original, pair_hash)
    for k, slot in enumerate(obj.material_slots[:len(keys)]):   # faces in added slots: mesh.matches
        mat = slot.material
        if mat is not None and mat.get('sbx_key') != keys[k]:
            return 'slot %d has another material (%s)' % (k + 1, mat.name)
    return None


def _encode(build, obj, i, why, mesh, unedited, pair_hash, targets=()):
    """The edited shape (and its tire morph targets) -> {shape index: new Shape}."""
    part_name, part = build.name, build.part
    original = unedited.shapes[i]
    keys = _original_keys(obj, original, pair_hash)
    records = _slot_records(build, obj, i, part.shapes[i].draws, keys)
    positions, normals, uvs, uvs2, colors, tris, slots = mesh.game_vertices()
    draws, used_slots = [], []
    info = build.slots[original.name] = SlotInfo(obj)
    for k, (rec, ref) in enumerate(records):
        t = tris[slots == k]
        if len(t):
            draws.append((rec, t))
            used_slots.append(k)
            mat = obj.material_slots[k].material
            info.draws.append((k, mat.name if mat else None, ref))
        else:
            log.info('%s %s: slot %d has no faces, its draw is left out', part_name, obj.name, k + 1)
    if (slots >= len(records)).any():
        raise ExportError('%s %s: faces use a material slot that doesn\'t exist' % (part_name, obj.name))
    out = {}
    try:
        out[i] = encode_shape(original, positions, normals, uvs, uvs2, colors, draws, mesh.uvs3)
        for t, name in targets:
            if name not in mesh.keys:
                raise ExportError('%s %s: shape key %s is missing (the tire needs every target)'
                                  % (part_name, obj.name, name))
            tpos, tnrm = mesh.remap(*mesh.keys[name])
            # A slot that kept its original draw takes the target's own
            # record for that draw; another material's record is shared.
            own = part.shapes[i].draws
            tdraws = [(part.shapes[t].draws[k] if k < len(own) and rec is own[k] else rec, tr)
                      for k, (rec, tr) in zip(used_slots, draws)]
            out[t] = encode_shape(unedited.shapes[t], tpos, tnrm, uvs, uvs2, colors, tdraws, mesh.uvs3)
    except EncodeError as e:
        raise ExportError('%s %s: edited (%s), but it can\'t be written: %s' % (part_name, obj.name, why, e))
    log.info('%s %s: edited (%s), encoded: %d vertices, %d draws, %d triangles%s',
             part_name, obj.name, why, len(positions), len(draws), len(tris),
             ', with %d morph targets' % len(targets) if targets else '')
    return out


def _encode_new(build, obj, root, name, unedited, apply_modifiers, depsgraph):
    """A new shape object -> its encoded Shape, ready for compose.add_shape."""
    part_name = build.name
    where = 'new shape %s (%s)' % (name, obj.name)
    if obj.data.shape_keys is not None:
        raise ExportError('%s %s: shape keys can\'t be written on a new shape' % (part_name, where))
    if not obj.material_slots or all(s.material is None for s in obj.material_slots):
        raise ExportError('%s %s: give it a material from an imported part, or one made with '
                          'Make SBX Material' % (part_name, where))
    mesh = _mesh_of(obj, root, apply_modifiers, depsgraph)
    positions, normals, uvs, uvs2, colors, tris, slots = mesh.game_vertices()
    if not len(tris):
        raise ExportError('%s %s: no faces' % (part_name, where))
    if (slots >= len(obj.material_slots)).any():
        raise ExportError('%s %s: faces use a material slot that doesn\'t exist' % (part_name, where))
    entry = _shape_entry(build.readable, new_name=name)
    info = SlotInfo(obj, new=True)
    draws, refs = [], []
    for k, slot in enumerate(obj.material_slots):
        t = tris[slots == k]
        if not len(t):
            continue   # an unused slot needs no SBX material
        swhere = '%s slot %d' % (where, k + 1)
        rec, ref = build.material(slot.material, swhere)
        build.entry_edits(rec, _draw_entry(entry, k, slot.material.name), swhere)
        draws.append((rec, t))
        refs.append(ref)
        info.draws.append((k, slot.material.name, ref))
    candidates = []
    for ref in dict.fromkeys(refs):
        try:
            candidates.append(build.sources.template_shape(ref))
        except SourceError as e:
            log.warning('%s %s: no layout from %s: %s', part_name, where, ref, e)
    candidates += list(unedited.shapes)
    position_types = collections.Counter(s.streams[1].type for s in unedited.shapes if s.streams[1] is not None)
    position_type = position_types.most_common(1)[0][0] if position_types else None
    try:
        template = choose_template(candidates, position_type)
        if uvs is None and template.streams[0].type != 0x00:
            log.warning('%s %s: the mesh has no UV map; written with zero UVs', part_name, where)
        if uvs2 is None and template.streams[0].type in (0x0F, 0x10):
            log.info('%s %s: the layout has a second UV map and the mesh one; the first is used twice',
                     part_name, where)
        raw_name = name.encode('ascii')
        if not raw_name or len(raw_name) > 31:
            raise ValueError('shape names are 1 to 31 characters')
        shape = encode_new_shape(template, raw_name + b'\0' * (32 - len(raw_name)), positions, normals,
                                 uvs, uvs2, colors, draws, mesh.uvs3)
    except (EncodeError, UnicodeEncodeError, ValueError) as e:
        raise ExportError('%s %s: can\'t be written: %s' % (part_name, where, e))
    info.template = template.name
    build.slots[name] = info
    log.info('%s %s: encoded in the layout of %s (%s): %d vertices, %d draws, %d triangles', part_name, where,
             template.name, ', '.join('0x%02X' % st.type for st in template.streams if st is not None),
             len(positions), len(draws), len(tris))
    return shape


def _markers(part_name, root, members, part, moved):
    """Moved marker empties of a part -> moved[embedded node index] = position."""
    if part.embedded_nodes is None:
        return
    nodes = part.embedded_nodes.nodes
    index_of = {n.name: n.get('node_index') for n in part.nodes if not n.is_shape_node}
    for c in members:
        name = c.get('sbx_marker')
        if name is None or name not in index_of:
            continue
        k = index_of[name]
        m = part_tree.to_root(c, root)
        pos = m[:3, 3] @ np.linalg.inv(frames.NODE_TO_BLENDER)
        if not np.allclose(m[:3, :3], np.eye(3), atol=1e-6):
            log.warning('%s marker %s: rotated or scaled; only its position is exported', part_name, name)
        if np.abs(pos - nodes[k].translation).max() > MARKER_TOLERANCE:
            if k in moved and np.abs(moved[k] - pos).max() > MARKER_TOLERANCE:
                raise ExportError('%s: imported more than once and marker %s differs between the copies'
                                  % (part_name, name))
            moved[k] = pos


def export_parts(context, folder, skip_identical=False, apply_modifiers=False):
    """Write every visible part into folder. Returns counts."""
    from . import edit_part
    os.makedirs(folder, exist_ok=True)
    roots = part_roots(context.scene)
    depsgraph = context.evaluated_depsgraph_get() if apply_modifiers else None
    counts = {'written': 0, 'identical': 0, 'skipped': 0, 'edited': 0, 'added': []}
    sources = Sources()
    log.info('export of %d visible parts to %s (apply modifiers: %s)', len(roots), folder, apply_modifiers)
    for name in sorted(roots):
        res = build_part(name, roots[name], apply_modifiers, depsgraph, sources)
        data, original_sha, changes = res.data, res.original_sha256, res.changes
        edit_part.write_readable(name, res)
        same = sidecar.sha256(data) == original_sha
        counts['identical'] += same
        counts['edited'] += bool(changes)
        counts['added'] += ['%s: %s' % (name, c) for c in changes if c.startswith(('new ', 'shader pair'))]
        for c in changes:
            log.info('%s: %s', name, c)
        if same and skip_identical:
            counts['skipped'] += 1
            log.info('%s: identical to the original, skipped', name)
            continue
        path = os.path.join(folder, name + '.part')
        with open(path, 'wb') as f:
            f.write(data)
        counts['written'] += 1
        log.info('%s: written, %d bytes, %s', name, len(data),
                 'identical to the original' if same else 'edited')
    return counts
