"""The decoded view of a PART: positions, normals, UVs, colors, triangles.

What the tools and the Blender import work with. Built from the
byte-exact model (part_model.PARTFile); nothing here is written back.
Shape matrices are applied (identity in every part seen), and the
strips are turned into triangles (degenerates dropped). Format:
docs/game_data.md, "Part files".

    part = decode_PART(read_PART(data))
    for shape in part.shapes:
        print(shape.name, len(shape.positions), len(shape.draws))
"""

import re
import struct

import numpy as np

from .strips import PRIM_TRIANGLE_STRIP

# Tire parts hold a base shape and targets with the same vertices and
# indices (the game blends between them; what each target is for isn't
# known).
MORPH_BASE = 'base_Shape'
MORPH_TARGET = re.compile(r'^target_\d+_Shape$')


class Draw:
    """One material record: one draw call of a shape.

    The meaning of the color fields is inferred from the values (and the
    shader constants' names); see docs/game_data.md, "Part files".
    """

    def __init__(self):
        self.blend = 0               # +0x04: 1 on glass, glows, LEDs (blended draw, inferred)
        self.diffuse = (1.0, 1.0, 1.0, 1.0)   # +0x08 (inferred)
        self.ambient = (0.0, 0.0, 0.0, 1.0)   # +0x18 (inferred)
        self.specular = (0.0, 0.0, 0.0, 1.0)  # +0x28 (inferred)
        self.power = 0.0             # +0x58, specular power (2, 20, 100; inferred)
        self.params = (0.0, 0.0)     # +0x5C, +0x60
        self.shader = 0              # +0x68: index of one of the part's shader pairs
        self.flags = 0               # +0x70
        self.shared_shader = 0       # +0x190: shared material shader pair (1000-1005) or 0
        self.textures = []           # [(XPR2 list index, texture index)] for each bound texture
        self.triangles = np.zeros((0, 3), np.uint32)

    @property
    def texture(self):
        """Index of the first bound texture, or -1."""
        return self.textures[0][1] if self.textures else -1


class Shape:
    def __init__(self, name):
        self.name = name
        self.matrix = np.eye(4)  # +0x020, identity in every RX7 part
        self.positions = None    # (n, 3) float
        self.normals = None      # (n, 3) float, or None
        self.uvs = None          # (n, 2) float, Direct3D orientation (v = 0 at the top)
        self.uvs2 = None
        self.uvs3 = None         # stream type 0x10 only (the same as uvs in every shape)
        self.colors = None       # (n, 4) uint8 as stored
        self.draws = []
        self.joints = None       # (n, k) bone indices into the model's node file, or None
        self.weights = None      # (n, k) float, 0 for unused influences

    @property
    def skinned(self):
        return self.joints is not None


class PartNode:
    """An entry of a part's own node list.

    kind is 'shape' (the node a shape hangs from; the name says what it
    attaches to: fb, fender, room ...) or 'marker' (an effect point such
    as an exhaust tip). position is the marker's translation from the
    part's embedded node file, in the node frame.
    """

    def __init__(self, name, kind, node_index, shape_index):
        self.name = name
        self.kind = kind
        self.node_index = node_index
        self.shape_index = shape_index
        self.position = None
        self.rotation = None


class Part:
    def __init__(self):
        self.nodes = []          # PartNode
        self.shapes = []         # Shape
        self.embedded_nodes = None   # list of Node from the embedded node file, or None
        self.shader_count = 0    # compiled VS/PS pairs in the part

    @property
    def attach(self):
        """Name of the node the part's shapes attach to, or None."""
        return next((n.name for n in self.nodes if n.kind == 'shape'), None)

    @property
    def markers(self):
        return [n for n in self.nodes if n.kind == 'marker']


def _draw(rec):
    m = Draw()
    m.blend = rec.get('blend')
    m.diffuse = rec.get('diffuse')
    m.ambient = rec.get('ambient')
    m.specular = rec.get('specular')
    m.power = rec.get('power')
    m.params = rec.get('params')
    m.shader = rec.get('shader')
    m.flags = rec.get('flags')
    m.shared_shader = rec.get('shared_shader')
    m.textures = rec.textures
    if rec.get('primitive_type') != PRIM_TRIANGLE_STRIP:
        raise ValueError('primitive type %d' % rec.get('primitive_type'))
    m.triangles = rec.triangles()
    return m


def _read_skin(d, block):
    """Skin block -> (joints, weights) as (n, k) arrays, or None if static.

    d is the skin block's own bytes; its offsets are file offsets, so
    block is the block's file offset.
    """
    count, _, counts_off, weights_off, joints_off = struct.unpack_from('>5I', d, 0)
    if not count or counts_off == block:
        return None
    rel = lambda o: o - block  # noqa: E731
    counts = np.frombuffer(d, '>u4', count, rel(counts_off)).astype(np.int64)
    total = int(counts.sum())
    w = np.frombuffer(d, '>f4', total * 4, rel(weights_off)).reshape(total, 4)[:, 0].astype(np.float64)
    j = np.frombuffer(d, '>u4', total, rel(joints_off)).astype(np.int64)
    k = int(counts.max())
    joints = np.zeros((count, k), np.int64)
    weights = np.zeros((count, k))
    starts = np.concatenate([[0], np.cumsum(counts)[:-1]])
    for slot in range(k):
        has = counts > slot
        joints[has, slot] = j[starts[has] + slot]
        weights[has, slot] = w[starts[has] + slot]
    return joints, weights


def decode_PART(pf):
    """PARTFile -> Part (decoded, shape matrices applied)."""
    part = Part()
    part.shader_count = pf.shader_count
    for n in pf.nodes:
        part.nodes.append(PartNode(n.name, 'shape' if n.get('kind') == 1 else 'marker',
                                   n.get('node_index'), n.get('shape_index')))
    if pf.embedded_nodes is not None:
        part.embedded_nodes = pf.embedded_nodes.nodes
        for n in part.nodes:
            if 0 <= n.node_index < len(part.embedded_nodes):
                e = part.embedded_nodes[n.node_index]
                n.position, n.rotation = e.translation, e.rotation

    for s in pf.shapes:
        shape = Shape(s.name)
        shape.matrix = s.matrix
        attrs = s.attributes()
        shape.positions = attrs.get('position')
        if shape.positions is None:
            raise ValueError('%s: no positions' % shape.name)
        shape.normals = attrs.get('normal')
        shape.uvs = attrs.get('uv')
        shape.uvs2 = attrs.get('uv2')
        shape.uvs3 = attrs.get('uv3')
        shape.colors = attrs.get('color')
        if not np.allclose(shape.matrix, np.eye(4)):
            m = shape.matrix
            shape.positions = shape.positions @ m[:3, :3] + m[3, :3]
            if shape.normals is not None:
                shape.normals = shape.normals @ m[:3, :3]
        skin = _read_skin(s.skin_raw, s.skin_source_offset)
        if skin is not None:
            shape.joints, shape.weights = skin
        for rec in s.draws:
            try:
                shape.draws.append(_draw(rec))
            except ValueError as e:
                raise ValueError('%s: %s' % (shape.name, e))
        part.shapes.append(shape)
    return part


def morph_groups(shapes):
    """Find tire-style morphs: {base shape index: [target shape indices]}.

    A target joins its base only when the vertex count, the UVs and every
    draw's triangles match.
    """
    groups = {}
    base = next((i for i, s in enumerate(shapes) if s.name == MORPH_BASE), None)
    if base is None:
        return groups
    b = shapes[base]
    for i, s in enumerate(shapes):
        if i == base or not MORPH_TARGET.match(s.name):
            continue
        if len(s.positions) != len(b.positions) or len(s.draws) != len(b.draws):
            continue
        if any(not np.array_equal(x.triangles, y.triangles) for x, y in zip(s.draws, b.draws)):
            continue
        if (s.uvs is None) != (b.uvs is None) or (s.uvs is not None and not np.array_equal(s.uvs, b.uvs)):
            continue
        groups.setdefault(base, []).append(i)
    return groups
