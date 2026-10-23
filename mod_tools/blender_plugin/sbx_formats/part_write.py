"""PART writer: part_model.PARTFile -> bytes.

Lays the file out again from the model, in the game's section order, and
recomputes every offset, size and count that depends on the layout. All
other bytes come from the model's raw records, so an untouched part
writes back identical to the file it was read from.

Layout (docs/game_data.md, "Part files", "Layout"): header, nodes,
shapes, material blocks, skin blocks, skin arrays, the embedded section
and the zeros after it, small sections, vertex streams (shape by shape,
slot by slot), index buffers (draw by draw, each padded to 4 bytes),
then the shader section on a 0x20 boundary.

What it recomputes:
- header: counts, the section offsets, the small offsets (anchors), the
  vertex data size (the game copies that many bytes of streams and index
  buffers: a stale size cuts off a bigger part's vertex data);
- shape: stream descriptors, the fetch constants' offset and size words,
  the stream, material, skin and skin array offsets;
- material block: count and size;
- material record: index count, primitive count, index offsets and size;
- skin block: its three internal offsets (moved with the block).

Not recomputed (written as stored): the shape bounds (the export of an
edited shape sets them) and everything named unknown_* in layout.py.
"""

import struct

from . import layout
from .part_model import PARTHeader, Shape, Draw

FETCH_OFFSET_TAG = 3   # fetch constant word 6: offset from the vertex data | 3
FETCH_SIZE_TAG = 2     # fetch constant word 7: size in bytes | 2 (endian swap 8in32)
SHADER_ALIGN = 0x20
INDEX_ALIGN = 4


def _pad(out, align):
    out.extend(b'\0' * (-len(out) % align))


def _patch(record_type, raw, values):
    """A record's raw bytes with fields replaced: values is {name: value}."""
    b = bytearray(raw)
    for name, value in values.items():
        _, off, fmt = record_type.field(name)
        if not isinstance(value, (tuple, list)):
            value = (value,)
        struct.pack_into('>' + fmt, b, off, *value)
    return bytes(b)


def _relocate_skin(raw, old, new):
    """A skin block moved from old to new: its three offsets move with it."""
    if len(raw) < 0x14:
        return raw
    b = bytearray(raw)
    for o in (0x08, 0x0C, 0x10):
        v = struct.unpack_from('>I', b, o)[0]
        if v:
            struct.pack_into('>I', b, o, v - old + new)
    return bytes(b)


def write_PART(part, _test_gap=0):
    """PARTFile -> bytes.

    _test_gap inserts that many zero bytes after the shape records, moving
    every section after them. Only for the relocation test: a file
    written with a gap must read back and write again identical.
    """
    out = bytearray(part.header.raw)
    shapes = part.shapes
    pos = {}   # piece key -> new offset (for the anchors)

    nodes_off = len(out)
    for n in part.nodes:
        out += n.raw

    shapes_off = len(out)
    out += b'\0' * (layout.SHAPE_SIZE * len(shapes))   # filled in at the end
    out += b'\0' * _test_gap

    block_off, draw_off = [], []
    for s in shapes:
        mb = s.material_block
        block_off.append(len(out))
        size = 8 + len(mb.draws) * layout.MATERIAL_SIZE + len(mb.tail)
        out += struct.pack('>2I', len(mb.draws), size) + mb.head[8:]
        offs = []
        for dr in mb.draws:
            offs.append(len(out))
            out += b'\0' * layout.MATERIAL_SIZE              # filled in at the end
        draw_off.append(offs)
        out += mb.tail
    materials_off = block_off[0] if shapes else shapes_off

    skin_off = []
    for s in shapes:
        skin_off.append(len(out))
        out += _relocate_skin(s.skin_raw, s.skin_source_offset, len(out))

    skin_array_off = []
    for s in shapes:
        a = len(out)
        out += s.skin_positions.raw
        b = len(out)
        out += s.skin_normals.raw
        skin_array_off.append((a, b))

    embedded_off = 0
    if part.embedded_raw:
        embedded_off = pos[('embedded',)] = len(out)
        out += part.embedded_raw + part.embedded_tail

    for k, (_, raw) in enumerate(part.small_sections):
        pos[('small', k)] = len(out)
        out += raw

    vertex_off = len(out)
    stream_off = []
    for i, s in enumerate(shapes):
        offs = [0, 0, 0]
        for k, st in enumerate(s.streams):
            if st is not None:
                offs[k] = pos[('stream', i, k)] = len(out)
                out += st.raw.tobytes()
        end = len(out)
        offs = [o if s.streams[k] is not None else end for k, o in enumerate(offs)]
        stream_off.append(offs)

    index_off = []
    for i, s in enumerate(shapes):
        offs = []
        for k, dr in enumerate(s.draws):
            offs.append(len(out))
            pos[('indices', i, k)] = len(out)
            out += dr.indices_raw
            _pad(out, INDEX_ALIGN)
        index_off.append(offs)

    vertex_size = len(out) - vertex_off
    shaders_off = 0
    if part.shaders_raw:
        _pad(out, SHADER_ALIGN)
        shaders_off = pos[('shaders',)] = len(out)
        out += part.shaders_raw

    # Shapes and their material records.
    for i, s in enumerate(shapes):
        v = {'stream_offsets': stream_off[i], 'material_offset': block_off[i],
             'skin_position_offset': skin_array_off[i][0],
             'skin_normal_offset': skin_array_off[i][1], 'skin_offset': skin_off[i]}
        for k, st in enumerate(s.streams):
            fetch = list(s.get('fetch_%d' % k))
            if st is not None:
                v['stream_%d' % k] = (st.count, st.type, st.stride, st.size)
                size = st.size
            else:
                size = 0
            fetch[6] = (stream_off[i][k] - vertex_off) | FETCH_OFFSET_TAG
            fetch[7] = size | FETCH_SIZE_TAG
            v['fetch_%d' % k] = fetch
        o = shapes_off + i * layout.SHAPE_SIZE
        out[o:o + layout.SHAPE_SIZE] = _patch(Shape, s.raw, v)

        for k, dr in enumerate(s.draws):
            count = len(dr.indices_raw) // 2
            raw = _patch(Draw, dr.raw, {
                'index_count': count,
                'primitive_count': max(count - 2, 0),
                'index_offset': index_off[i][k] - vertex_off,
                'index_size': -(-2 * count // INDEX_ALIGN) * INDEX_ALIGN,
                'index_file_offset': index_off[i][k]})
            o = draw_off[i][k]
            out[o:o + layout.MATERIAL_SIZE] = raw

    small = []
    for a in part.small_slots:
        small.append(0 if a is None else pos[a[0]] + a[1])

    out[:layout.HEADER_SIZE] = _patch(PARTHeader, part.header.raw, {
        'node_count': len(part.nodes),
        'shape_count': len(shapes),
        'nodes_offset': nodes_off,
        'nodes_offset_2': nodes_off,
        'shapes_offset': shapes_off,
        'materials_offset': materials_off,
        'embedded_offset': embedded_off,
        'small_offsets': small,
        'vertex_data_offset': vertex_off,
        'vertex_data_size': vertex_size,
        'shaders_offset': shaders_off})
    return bytes(out)
