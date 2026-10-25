"""Encoding an edited shape: vertex data and triangles -> PART bytes.

    shape = encode_shape(original_shape, positions, normals, uvs, uvs2,
                         colors, draws)

Everything is in the game's terms: car space, Direct3D UVs (v = 0 at the
top), colors as stored (ARGB bytes), triangles in the game's winding.
draws is [(material record (part_model.Draw), triangles)] in the order
they go into the material block. The shape keeps its original stream
layout (types, strides, slots); the writer recomputes the offsets and
sizes.

Rules found in the data (docs/game_data.md, "Part files"; all 19,396
shapes):
- the bounding box spans the vertices and the origin, corners in the
  order bounds_corners gives; the sphere is the box's center and half
  its diagonal;
- the skin block's first word and the skin arrays' counts are the
  vertex count, and a skin array is a copy of the vertex stream of the
  same type.

Shapes with stream types not worked out (0x0D), skinned shapes and
shapes with more than 65,535 vertices can't be encoded (EncodeError).
Stream type 0x10's third UV is written as the first unless uvs3 is given
(it's the first in every shape of the game's parts).
"""

import struct

import numpy as np

from . import streams as S
from .part_model import Draw, MaterialBlock, Shape, SkinArray
from .strips import RESTART
from .streams import Stream

MAX_VERTICES = 0xFFFF   # 16-bit indices, 0xFFFF is the strip restart


class EncodeError(Exception):
    pass


# ---------------------------------------------------------------- strips

def stripify(triangles):
    """(n, 3) triangles in the game's winding -> strip indices with 0xFFFF
    restarts, which strip_to_triangles turns back into the same triangles
    (same winding; the order may differ). Greedy: each strip grows while a
    triangle shares its last edge the right way round."""
    tris = [tuple(int(v) for v in t) for t in triangles
            if t[0] != t[1] and t[1] != t[2] and t[0] != t[2]]
    # Directed edge (u, v) -> triangles whose winding has u then v.
    by_edge = {}
    for k, (a, b, c) in enumerate(tris):
        for u, v, w in ((a, b, c), (b, c, a), (c, a, b)):
            by_edge.setdefault((u, v), []).append((k, w))
    used = [False] * len(tris)
    out = []

    def take(u, v):
        for k, w in by_edge.get((u, v), ()):
            if not used[k]:
                used[k] = True
                return w
        return None

    for k, (a, b, c) in enumerate(tris):
        if used[k]:
            continue
        used[k] = True
        strip = [a, b, c]
        while True:
            n = len(strip) - 2            # index of the next triangle
            p, q = strip[-2], strip[-1]
            # Triangle n is (i[n], i[n+1], i[n+2]), the first two swapped
            # when n is odd.
            w = take(q, p) if n % 2 else take(p, q)
            if w is None:
                break
            strip.append(w)
        if out:
            out.append(RESTART)
        out.extend(strip)
    return np.array(out, np.uint16)


def strip_bytes(indices):
    return np.asarray(indices, '>u2').tobytes()


# ---------------------------------------------------------------- bounds

def bounds(positions):
    """(box: 8 x float4, sphere: center xyz, radius) as the game's files have them."""
    lo = np.minimum(positions.min(0), 0.0) if len(positions) else np.zeros(3)
    hi = np.maximum(positions.max(0), 0.0) if len(positions) else np.zeros(3)
    box = [[lo[0] if i & 1 else hi[0], hi[1] if i & 4 else lo[1], hi[2] if i & 2 else lo[2], 1.0]
           for i in range(8)]
    center = (lo + hi) / 2.0
    return box, [*center, float(np.linalg.norm(hi - lo) / 2.0)]


# ---------------------------------------------------------------- streams

def _half(a):
    return np.asarray(a, np.float32).astype('>f2')


def encode_stream(stype, stride, n, positions, normals, uvs, uvs2, colors, uvs3=None):
    """One stream of a known type -> (n, stride) uint8."""
    out = np.zeros((n, stride), np.uint8)
    if stype in (S.COLOR, S.COLOR_UV, S.COLOR_UV2, S.COLOR_UV3):
        c = colors if colors is not None else np.full((n, 4), 255, np.uint8)
        out[:, 0:4] = c
        if stype != S.COLOR:
            if uvs is None:
                raise EncodeError('the stream needs UVs and the mesh has none')
            out[:, 4:8] = _half(uvs).view(np.uint8).reshape(n, 4)
        if stype in (S.COLOR_UV2, S.COLOR_UV3):
            if uvs2 is None:
                raise EncodeError('the stream needs a second UV map and the mesh has only one')
            out[:, 8:12] = _half(uvs2).view(np.uint8).reshape(n, 4)
        if stype == S.COLOR_UV3:
            # UV3 is UV1 in every shape of the game's parts.
            out[:, 12:16] = _half(uvs if uvs3 is None else uvs3).view(np.uint8).reshape(n, 4)
    elif stype == S.POSITION:
        out[:] = np.asarray(positions, '>f4').view(np.uint8).reshape(n, 12)
    elif stype == S.NORMAL:
        if normals is None:
            raise EncodeError('the stream needs normals')
        h = np.ones((n, 4), np.float32)
        h[:, :3] = normals
        out[:] = _half(h).view(np.uint8).reshape(n, 8)
    elif stype in (S.SKIN_POSITION, S.SKIN_NORMAL):
        src = positions if stype == S.SKIN_POSITION else normals
        if src is None:
            raise EncodeError('the stream needs normals')
        f = np.ones((n, 4), np.float32)
        f[:, :3] = src
        out[:] = np.asarray(f, '>f4').view(np.uint8).reshape(n, 16)
    else:
        raise EncodeError('stream type 0x%02X (stride %d) isn\'t worked out' % (stype, stride))
    return out


# ---------------------------------------------------------------- shape

def encode_shape(original, positions, normals, uvs, uvs2, colors, draws, uvs3=None):
    """A new part_model.Shape from edited data; original gives the record,
    the stream layout and the skin data."""
    n = len(positions)
    if n == 0:
        raise EncodeError('no vertices')
    if n > MAX_VERTICES:
        raise EncodeError('%d vertices, at most %d' % (n, MAX_VERTICES))
    if original.unknown_streams():
        raise EncodeError('stream type not worked out (0x%02X)' % original.unknown_streams()[0].type)
    if int.from_bytes(original.skin_raw[4:8], 'big') != 0x20:
        raise EncodeError('skinned shape (bones)')
    if not np.allclose(original.matrix, np.eye(4)):
        raise EncodeError('shape matrix isn\'t identity')
    positions = np.asarray(positions, np.float64)

    shape = Shape(original.raw)
    box, sphere = bounds(positions)
    shape.set('bounds_box', [float(x) for row in box for x in row])
    shape.set('bounds_sphere', [float(x) for x in sphere])
    for k, st in enumerate(original.streams):
        if st is not None:
            raw = encode_stream(st.type, st.stride, n, positions, normals, uvs, uvs2, colors, uvs3)
            shape.streams[k] = Stream(st.type, st.stride, raw)

    def skin_array(a):
        if a.stream is None:
            return SkinArray(a.descriptor, None)
        same = next(s for s in shape.streams if s is not None and s.type == a.stream.type)
        return SkinArray(struct.pack('>4I', n, same.type, same.stride, same.raw.size), same)

    shape.skin_positions = skin_array(original.skin_positions)
    shape.skin_normals = skin_array(original.skin_normals)
    shape.skin_raw = struct.pack('>I', n) + original.skin_raw[4:]
    shape.skin_source_offset = original.skin_source_offset

    records = []
    for record, tris in draws:
        if np.asarray(tris).size and int(np.asarray(tris).max()) >= n:
            raise EncodeError('a triangle uses vertex %d of %d' % (int(np.asarray(tris).max()), n))
        records.append(Draw(record.raw, strip_bytes(stripify(tris))))
    mb = original.material_block
    shape.material_block = MaterialBlock(mb.head, records, mb.tail)
    return shape


# ---------------------------------------------------------------- new shapes

# How much of the color/UV stream a layout carries: a new shape takes the
# layout with the most, so every draw's shader finds what it reads.
UV_RANK = {S.COLOR: 0, S.COLOR_UV: 1, S.COLOR_UV2: 2, S.COLOR_UV3: 3}


def layout_of(shape):
    return tuple(st.type if st is not None else None for st in shape.streams)


def template_problem(shape):
    """Why a shape can't lend its layout to a new shape, or None."""
    if shape.unknown_streams():
        return 'stream type 0x%02X not worked out' % shape.unknown_streams()[0].type
    if int.from_bytes(shape.skin_raw[4:8], 'big') != 0x20:
        return 'skinned (bones)'
    if not np.allclose(shape.matrix, np.eye(4)):
        return 'shape matrix isn\'t identity'
    if shape.streams[0] is None or shape.streams[0].type not in UV_RANK:
        return 'no color/UV stream in slot 0'
    return None


def choose_template(candidates, position_type=None):
    """The shape a new shape copies its layout from.

    candidates: shapes whose layout fits (the shapes the new shape's
    materials came from, then the part's own). position_type: the
    position stream type the part's shapes use (0x0B, or 0x05 in the
    wheel parts); a candidate with it wins. Among those, the one whose
    color/UV stream carries the most (UV_RANK), then one with normals.
    Raises EncodeError if no candidate fits."""
    ok = [s for s in candidates if template_problem(s) is None]
    if not ok:
        why = '; '.join(sorted({template_problem(s) for s in candidates})) or 'none given'
        raise EncodeError('no shape to copy the stream layout from (%s)' % why)
    if position_type is not None and any(s.streams[1] is not None and s.streams[1].type == position_type
                                         for s in ok):
        ok = [s for s in ok if s.streams[1] is not None and s.streams[1].type == position_type]
    return max(ok, key=lambda s: (UV_RANK[s.streams[0].type], s.streams[2] is not None))


def encode_new_shape(template, name_raw, positions, normals, uvs, uvs2, colors, draws, uvs3=None):
    """A new shape in template's layout. name_raw: the 32-byte name.

    Data the layout needs and the mesh lacks is filled in: a second UV map
    is the first, a mesh without UVs gets zeros (the caller logs both);
    normals are needed only when the layout has them."""
    n = len(positions)
    st0 = template.streams[0]
    if st0 is not None and st0.type != S.COLOR:
        if uvs is None:
            uvs = np.zeros((n, 2))
        if st0.type in (S.COLOR_UV2, S.COLOR_UV3) and uvs2 is None:
            uvs2 = uvs
    shape = encode_shape(template, positions, normals, uvs, uvs2, colors, draws, uvs3)
    shape.set('name', name_raw)
    return shape
