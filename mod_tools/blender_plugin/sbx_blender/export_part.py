"""PART export: Blender objects -> part files.

    export_parts(context, folder, skip_identical=False, apply_modifiers=False)

Which parts: every part root (an empty tagged sbx_part, made by the
import) with at least one visible shape object. A part goes out whole:
its hidden shapes are written too, since the game finds shapes by name.

How each part is built:
1. Its sidecar is loaded and the edits made in the readable half are
   applied to the records (sidecar.apply_readable_edits): colors, power,
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
   slot) the record the material was made from is used (its sbx_record),
   with its shader found among this part's own shader pairs. Slots with no
   faces left are dropped.

Tire morphs: the tires' target shapes are shape keys of base_Shape. An
edited tire writes base_Shape from the mesh and each target from its
shape key (same name), all with the same vertex split.

Markers (empties tagged sbx_marker) that were moved are written into
the part's embedded node file (position only; a rotation is ignored,
with a warning).

Not possible yet (the export stops and says why): editing the two
shapes with stream type 0x0D (one bonnet model), shape keys
on shapes other than the tire morphs, skinned shapes, more than 65,535
vertices, and a material whose shader pair this part doesn't have.

apply_modifiers uses the evaluated mesh (modifiers and all); otherwise
modifiers are ignored. Files are named <part>.part (the patch loader
ignores the extension; docs/game_data.md, "Mod data"). A part imported
more than once (the wheels, at each tire) is written once; if its
copies differ, the export stops.
"""

import json
import os

import bpy
import numpy as np

from . import frames, properties
from .log import get_logger
from .materials import material_key
from .sbx_formats import read_PART, shaders, sidecar
from .sbx_formats.decode import decode_PART, morph_groups
from .sbx_formats.node import set_node_translation
from .sbx_formats.encode import EncodeError, encode_shape
from .sbx_formats.part_model import Draw
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
        if obj.type != 'EMPTY' or 'sbx_part' not in obj or 'sbx_sidecar' not in obj:
            continue
        shapes = [c for c in obj.children if properties.shape_ref(c)]
        if any(c.visible_get() for c in shapes):
            roots.setdefault(obj['sbx_part'], []).append(obj)
    return roots


def load_sidecar(part_name):
    text = bpy.data.texts.get(properties.sidecar_text_name(part_name))
    raw = properties.sidecar_raw(bpy.data.texts, part_name)
    if text is None or raw is None:
        raise ExportError('%s: its sidecar text blocks are missing (import the part again)' % part_name)
    return sidecar.join(json.loads(text.as_string()), raw)


# ---------------------------------------------------------------- mesh

class MeshData:
    """A shape object's mesh in car space, per vertex and per corner."""

    def __init__(self, obj, me):
        self.obj = obj
        self.fingerprint = properties.normals_fingerprint(me)
        n = len(me.vertices)
        co = np.zeros(n * 3, np.float32)
        me.vertices.foreach_get('co', co)
        co = co.reshape(n, 3).astype(np.float64)
        local = np.array(obj.matrix_parent_inverse @ obj.matrix_basis)
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


def _mesh_of(obj, apply_modifiers, depsgraph):
    """(MeshData, cleanup)."""
    if apply_modifiers and obj.modifiers:
        ev = obj.evaluated_get(depsgraph)
        me = ev.to_mesh()
        try:
            return MeshData(obj, me)
        finally:
            ev.to_mesh_clear()
    if obj.modifiers:
        log.warning('%s: modifiers ignored (Apply modifiers is off)', obj.name)
    return MeshData(obj, obj.data)


# ---------------------------------------------------------------- records

class RecordSource:
    """Material records by sbx_record ('part:shape:draw'), from the sidecars."""

    def __init__(self):
        self.sidecars = {}

    def get(self, ref):
        part_name, shape, draw = ref.rsplit(':', 2)
        if part_name not in self.sidecars:
            self.sidecars[part_name] = load_sidecar(part_name)
        sc = self.sidecars[part_name]
        raw = sc['raw']
        rec = Draw(sidecar.unb64(raw['shapes'][int(shape)]['draws'][int(draw)]['record']))
        _, pairs, _ = shaders.split_section(sidecar.unb64(raw['shaders']))
        sh = rec.get('shader')
        return rec, (pairs[sh].ids if sh < len(pairs) else None)


def _slot_records(obj, part_name, shape_index, original_records, original_keys, pair_ids, sources):
    """The record for each of the object's material slots."""
    out = []
    for k, slot in enumerate(obj.material_slots):
        mat = slot.material
        if k < len(original_records) and (mat is None or mat.get('sbx_key') == original_keys[k]):
            out.append(original_records[k])
            continue
        if mat is None or 'sbx_record' not in mat:
            raise ExportError('%s %s: material slot %d has no SBX material (use one from an imported part)'
                              % (part_name, obj.name, k + 1))
        rec, ids = sources.get(mat['sbx_record'])
        rec = Draw(rec.raw)
        if ids not in pair_ids:
            raise ExportError('%s %s: material %s uses shader %s, which this part doesn\'t have (%s); '
                              'the shader library is needed for that (later)'
                              % (part_name, obj.name, mat.name, ids, ', '.join(pair_ids)))
        rec.set('shader', pair_ids.index(ids))
        log.info('%s %s: slot %d takes the record of %s (%s)', part_name, obj.name, k + 1, mat.name,
                 mat['sbx_record'])
        out.append(rec)
    return out


# ---------------------------------------------------------------- parts

def build_part(part_name, roots, apply_modifiers=False, depsgraph=None, sources=None):
    """A part's file bytes from its sidecar and its shape objects.
    Returns (data, original sha256, [what changed])."""
    sources = sources or RecordSource()
    sc = load_sidecar(part_name)
    unedited = sidecar.from_sidecar(sc)        # records before the readable edits
    changes = sidecar.apply_readable_edits(sc)
    part = sidecar.from_sidecar(sc)
    decoded = decode_PART(unedited)
    _, pairs, _ = shaders.split_section(part.shaders_raw)
    pair_ids = [p.ids for p in pairs]
    pair_hash = [p.vs_hash + ':' + p.ps_hash for p in pairs]

    morphs = morph_groups(decoded.shapes)
    results = {}   # shape index -> (root name, new shape or None)
    moved_markers = {}   # embedded node index -> position (node frame)
    for root in roots:
        _markers(part_name, root, part, decoded, moved_markers)
        by_index = {}
        for c in root.children:
            ref = properties.shape_ref(c)
            if ref and ref[0] == part_name:
                by_index.setdefault(ref[1], []).append(c)
        for i, shape in enumerate(decoded.shapes):
            objs = by_index.get(i, [])
            if len(objs) > 1:
                raise ExportError('%s: %d objects claim shape %d (%s)' % (part_name, len(objs), i, shape.name))
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
            mesh = _mesh_of(obj, apply_modifiers, depsgraph)
            why = mesh.matches(shape)
            if why is None:
                for t in targets:
                    key = mesh.keys.get(decoded.shapes[t].name)
                    if key is None:
                        why = 'shape key %s missing' % decoded.shapes[t].name
                    elif np.abs(key[0] - decoded.shapes[t].positions).max(initial=0.0) > POSITION_TOLERANCE:
                        why = 'shape key %s changed' % decoded.shapes[t].name
            new = {}
            if why is not None:
                new = _encode(part_name, obj, i, why, mesh, unedited, part, pair_ids, pair_hash, sources,
                              [(t, decoded.shapes[t].name) for t in targets])
                changes.append('%s: %s' % (shape.name, why))
            if i in results and (not results[i][1]) != (not new):
                raise ExportError('%s: imported more than once and the copies differ (%s); edit one copy only '
                                  'and delete the others' % (part_name, shape.name))
            results[i] = (root.name, new)
    for i, (_, new) in results.items():
        for k, shape in new.items():
            part.shapes[k] = shape
    for index, pos in moved_markers.items():
        part.embedded_raw = set_node_translation(part.embedded_raw, index, pos)
        changes.append('marker %s moved' % part.embedded_nodes.nodes[index].name)
    data = write_PART(part)
    read_PART(data)   # it must read back
    return data, sc['part'].get('sha256'), changes


def _encode(part_name, obj, i, why, mesh, unedited, part, pair_ids, pair_hash, sources, targets=()):
    """The edited shape (and its tire morph targets) -> {shape index: new Shape}."""
    original = unedited.shapes[i]
    keys = [material_key(r, pair_hash[r.get('shader')] if r.get('shader') < len(pair_hash) else None,
                         obj.material_slots[k].material.get('sbx_xpr', '')
                         if k < len(obj.material_slots) and obj.material_slots[k].material else '')
            for k, r in enumerate(original.draws)]
    records = _slot_records(obj, part_name, i, part.shapes[i].draws, keys, pair_ids, sources)
    positions, normals, uvs, uvs2, colors, tris, slots = mesh.game_vertices()
    draws, used_slots = [], []
    for k, rec in enumerate(records):
        t = tris[slots == k]
        if len(t):
            draws.append((rec, t))
            used_slots.append(k)
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
    log.info('%s %s: edited (%s), encoded: %d vertices (was %d), %d draws, %d triangles%s',
             part_name, obj.name, why, len(positions), len(original.streams[1].raw if original.streams[1] is not None else []),
             len(draws), len(tris), ', with %d morph targets' % len(targets) if targets else '')
    return out


def _markers(part_name, root, part, decoded, moved):
    """Moved marker empties under root -> moved[embedded node index] = position."""
    if part.embedded_nodes is None:
        return
    nodes = part.embedded_nodes.nodes
    index_of = {n.name: n.get('node_index') for n in part.nodes if not n.is_shape_node}
    for c in root.children:
        name = c.get('sbx_marker')
        if name is None or name not in index_of:
            continue
        k = index_of[name]
        m = np.array(c.matrix_basis)
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
    os.makedirs(folder, exist_ok=True)
    roots = part_roots(context.scene)
    depsgraph = context.evaluated_depsgraph_get() if apply_modifiers else None
    counts = {'written': 0, 'identical': 0, 'skipped': 0, 'edited': 0}
    sources = RecordSource()
    log.info('export of %d visible parts to %s (apply modifiers: %s)', len(roots), folder, apply_modifiers)
    for name in sorted(roots):
        data, original_sha, changes = build_part(name, roots[name], apply_modifiers, depsgraph, sources)
        same = sidecar.sha256(data) == original_sha
        counts['identical'] += same
        counts['edited'] += bool(changes)
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
