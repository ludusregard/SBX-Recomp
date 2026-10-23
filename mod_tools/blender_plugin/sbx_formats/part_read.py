"""PART reader: bytes -> part_model.PARTFile.

Reads everything into the model without judging it: unknown stream types,
primitive types and sections are kept raw, never an error. Only a wrong
magic is an error. Format: docs/game_data.md, "Part files".
"""

import struct

from . import layout, node
from .binary import u32
from .part_model import (Draw, MaterialBlock, PARTFile, PARTHeader, PART_MAGIC,
                         PartNode, Shape, SkinArray, find_anchor)
from .streams import read_stream

SMALL_SECTION_SIZE = 0x20


def is_PART(d):
    return d[:4] == PART_MAGIC


def _read_material_block(d, off):
    count, size = struct.unpack_from('>2I', d, off)
    draws = []
    for k in range(count):
        r = off + 8 + k * layout.MATERIAL_SIZE
        dr = Draw(d[r:r + layout.MATERIAL_SIZE])
        ioff, icount = dr.get('index_file_offset'), dr.get('index_count')
        dr.indices_raw = d[ioff:ioff + 2 * icount]
        draws.append(dr)
    tail_start = off + 8 + count * layout.MATERIAL_SIZE
    mb = MaterialBlock(d[off:off + 8], draws, d[tail_start:off + size])
    mb.source_offset = off
    return mb


def _read_skin_array(d, off):
    count, stype, stride, size = struct.unpack_from('>4I', d, off)
    a = SkinArray(d[off:off + 16], read_stream(d, off + 16, count, stype, stride, size) if count else None)
    a.source_offset = off
    return a


def read_PART(d):
    """PART file bytes -> PARTFile."""
    d = bytes(d)
    if not is_PART(d):
        raise ValueError('not a part file (magic %s)' % d[:4].hex())
    part = PARTFile()
    part.source_size = len(d)
    h = part.header = PARTHeader(d[:layout.HEADER_SIZE])

    base = h.get('nodes_offset')
    for i in range(h.get('node_count')):
        o = base + i * layout.PART_NODE_SIZE
        part.nodes.append(PartNode(d[o:o + layout.PART_NODE_SIZE]))

    base = h.get('shapes_offset')
    for i in range(h.get('shape_count')):
        o = base + i * layout.SHAPE_SIZE
        s = Shape(d[o:o + layout.SHAPE_SIZE])
        offs = s.get('stream_offsets')
        for k in range(3):
            count, stype, stride, size = s.get('stream_%d' % k)
            if count:
                s.streams[k] = read_stream(d, offs[k], count, stype, stride, size)
        s.material_block = _read_material_block(d, s.get('material_offset'))
        s.skin_positions = _read_skin_array(d, s.get('skin_position_offset'))
        s.skin_normals = _read_skin_array(d, s.get('skin_normal_offset'))
        sk = s.get('skin_offset')
        s.skin_raw = d[sk:sk + u32(d, sk + 4)]
        s.skin_source_offset = sk
        part.shapes.append(s)

    sh = h.get('shaders_offset')
    if sh:
        part.shaders_raw = d[sh:]

    # The small offsets: a small section each, unless the offset is where
    # another piece starts (part 141: the embedded section and a stream).
    small = sorted(set(o for o in h.get('small_offsets') if o))
    e = h.get('embedded_offset')
    starts = {start for _, start, _ in part.pieces()}
    if e:
        starts.add(e)
    part.small_sections = [(o, d[o:o + SMALL_SECTION_SIZE]) for o in small if o not in starts]

    if e:
        if node.is_NODE(d, e):
            part.embedded_nodes = node.read_NODE(d, e)
            part.embedded_raw = part.embedded_nodes.raw
        else:
            # Not a NODE file (one part): it runs to the next piece.
            later = [o for o in small + [h.get('vertex_data_offset'), sh] if o > e]
            part.embedded_raw = d[e:min(later) if later else len(d)]
        # Zero bytes between the embedded section and the next piece.
        end = e + len(part.embedded_raw)
        later = [o for o in small + [h.get('vertex_data_offset'), sh] if o >= end]
        nxt = min(later) if later else len(d)
        part.embedded_tail = d[end:nxt]
        if any(part.embedded_tail):
            raise ValueError('nonzero bytes after the embedded section at 0x%X' % end)

    pieces = part.pieces()
    part.small_slots = [find_anchor(pieces, o) if o else None for o in h.get('small_offsets')]
    if any(o and a is None for o, a in zip(h.get('small_offsets'), part.small_slots)):
        raise ValueError('a small offset points at no piece')
    return part
