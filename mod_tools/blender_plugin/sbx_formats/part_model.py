"""The in-memory model of a PART file (00 15 00 00).

Every record is kept as its raw bytes, with named fields decoded on
demand through the layout tables (layout.py). Sections that aren't worked
out are kept whole as raw bytes. So a PART read into this model holds
every byte of the file, and writing it back unchanged gives the same file.

Each piece remembers the offset it was read from (source_offset). The
writer lays the file out again from the model; source offsets are only for
checks and the diff. Format: docs/game_data.md, "Part files".
"""

import numpy as np

from . import layout
from .binary import Record, cstr
from .strips import strip_to_triangles

PART_MAGIC = b'\x00\x15\x00\x00'


class PARTHeader(Record):
    FIELDS = layout.HEADER_FIELDS
    SIZE = layout.HEADER_SIZE


class PartNode(Record):
    """An entry of the part's own node list (192 bytes).

    kind 1 is the node a shape hangs from (its name says what it attaches
    to: fb, fender, room ...); 0 is a marker such as an exhaust tip.
    """

    FIELDS = layout.PART_NODE_FIELDS
    SIZE = layout.PART_NODE_SIZE

    @property
    def name(self):
        return cstr(self.get('name'))

    @property
    def is_shape_node(self):
        return self.get('kind') == 1


class Draw(Record):
    """One material record (0x200 bytes) and its index buffer: one draw call.

    indices_raw is the index buffer exactly as stored: index_count
    big-endian u16 (triangle strips, 0xFFFF restarts).
    """

    FIELDS = layout.MATERIAL_FIELDS
    SIZE = layout.MATERIAL_SIZE

    def __init__(self, raw, indices_raw=b''):
        super().__init__(raw)
        self.indices_raw = bytes(indices_raw)

    @property
    def indices(self):
        return np.frombuffer(self.indices_raw, '>u2').astype(np.uint32)

    def triangles(self):
        return strip_to_triangles(self.indices)

    @property
    def textures(self):
        """[(XPR2 list index, texture index)] for each bound texture."""
        count = min(self.get('texture_count'), 8)
        xprs = self.get('texture_xprs')
        texs = self.get('texture_indices')
        return [(xprs[k], texs[k]) for k in range(count) if texs[k] >= 0]


class MaterialBlock:
    """A shape's material block: count, size, the records, then a tail.

    head is the first 8 bytes (count, size); tail is what follows the
    records up to the block's size (8 zero bytes in every part seen).
    """

    def __init__(self, head, draws, tail):
        self.head = bytes(head)
        self.draws = draws
        self.tail = bytes(tail)
        self.source_offset = None


class SkinArray:
    """A skinned position or normal array: a 16-byte descriptor (count,
    type, stride, size), then the data (none when count is 0)."""

    def __init__(self, descriptor, stream):
        self.descriptor = bytes(descriptor)
        self.stream = stream   # streams.Stream, or None when count is 0
        self.source_offset = None

    @property
    def raw(self):
        return self.descriptor + (self.stream.raw.tobytes() if self.stream else b'')


class Shape(Record):
    """A shape: its 0x220-byte record, its streams, draws and skin data.

    The attribute properties (positions, normals ...) are as stored, in
    the shape's own frame: the shape matrix is not applied.
    """

    FIELDS = layout.SHAPE_FIELDS
    SIZE = layout.SHAPE_SIZE

    def __init__(self, raw):
        super().__init__(raw)
        self.streams = [None, None, None]   # streams.Stream for each used slot
        self.material_block = None
        self.skin_positions = None           # SkinArray
        self.skin_normals = None             # SkinArray
        self.skin_raw = b''                  # skin block, not decoded here
        self.skin_source_offset = None

    @property
    def name(self):
        return cstr(self.get('name'))

    @property
    def matrix(self):
        return np.array(self.get('matrix'), np.float64).reshape(4, 4)

    @property
    def draws(self):
        return self.material_block.draws if self.material_block else []

    def attributes(self):
        """Decoded attributes: the streams, then the skin arrays over them
        (skinned shapes keep their bind-pose positions and normals there).
        Unknown stream types add nothing."""
        out = {}
        for s in self.streams:
            if s is not None:
                out.update(s.decode())
        for a in (self.skin_positions, self.skin_normals):
            if a is not None and a.stream is not None:
                out.update(a.stream.decode())
        return out

    def unknown_streams(self):
        return [s for s in self.streams if s is not None and not s.known]


class PARTFile:
    """A whole PART file.

    embedded_raw is the embedded section at the header's embedded offset
    (a NODE file in all parts but one); embedded_nodes is that NODE file
    read, or None; embedded_tail is the zero bytes after it (16 after a
    NODE file). small_sections are the sections at the header's six small
    offsets (0x20 bytes each, not worked out), in file order. shaders_raw
    is the shader section to the end of the file.

    Pointers whose target isn't worked out are kept as anchors: the piece
    they point into and the distance from its start, so the writer can
    point them at the same byte after laying the file out again. A piece
    is ('embedded',), ('small', k), ('stream', shape, slot),
    ('indices', shape, draw) or ('shaders',).

    small_slots: the anchor of each of the header's six small offsets
    (most point at a small section; in one part they point at the
    embedded section and a stream).
    """

    def __init__(self):
        self.header = None
        self.nodes = []
        self.shapes = []
        self.embedded_raw = b''
        self.embedded_tail = b''
        self.embedded_nodes = None          # node.NODEFile, or None
        self.small_sections = []            # [(source offset, raw)] in file order
        self.small_slots = [None] * 6       # anchors
        self.shaders_raw = b''
        self.source_size = 0

    @property
    def shader_count(self):
        """Compiled VS/PS pairs in the part (the shader section's first word)."""
        if len(self.shaders_raw) >= 4:
            return int.from_bytes(self.shaders_raw[:4], 'big')
        return 0

    def regions(self):
        """Every piece of the file at its source offset: [(offset, bytes, label)].

        Used by the round-trip check (rebuild the file from the pieces) and
        by the diff (name the piece a byte belongs to).
        """
        h = self.header
        out = [(0, h.raw, 'header')]
        base = h.get('nodes_offset')
        for i, n in enumerate(self.nodes):
            out.append((base + i * layout.PART_NODE_SIZE, n.raw, 'node %d' % i))
        base = h.get('shapes_offset')
        for i, s in enumerate(self.shapes):
            tag = 'shape %d (%s)' % (i, s.name)
            out.append((base + i * layout.SHAPE_SIZE, s.raw, tag))
            mb = s.material_block
            o = mb.source_offset
            out.append((o, mb.head, tag + ' material block head'))
            for k, dr in enumerate(mb.draws):
                out.append((o + 8 + k * layout.MATERIAL_SIZE, dr.raw, tag + ' material %d' % k))
                out.append((dr.get('index_file_offset'), dr.indices_raw, tag + ' indices %d' % k))
            out.append((o + 8 + len(mb.draws) * layout.MATERIAL_SIZE, mb.tail, tag + ' material block tail'))
            offs = s.get('stream_offsets')
            for k, st in enumerate(s.streams):
                if st is not None:
                    out.append((offs[k], st.raw.tobytes(), tag + ' stream %d' % k))
            for name, a in (('skin positions', s.skin_positions), ('skin normals', s.skin_normals)):
                out.append((a.source_offset, a.raw, tag + ' ' + name))
            out.append((s.skin_source_offset, s.skin_raw, tag + ' skin'))
        if self.embedded_raw:
            out.append((h.get('embedded_offset'), self.embedded_raw, 'embedded'))
            if self.embedded_tail:
                out.append((h.get('embedded_offset') + len(self.embedded_raw), self.embedded_tail, 'embedded tail'))
        for k, (o, raw) in enumerate(self.small_sections):
            out.append((o, raw, 'small section %d' % k))
        if self.shaders_raw:
            out.append((h.get('shaders_offset'), self.shaders_raw, 'shaders'))
        return out

    def pieces(self):
        """The pieces anchors can point into: [(key, source offset, size)]."""
        h = self.header
        out = []
        if self.embedded_raw:
            out.append((('embedded',), h.get('embedded_offset'), len(self.embedded_raw)))
        for k, (o, raw) in enumerate(self.small_sections):
            out.append((('small', k), o, len(raw)))
        for i, s in enumerate(self.shapes):
            offs = s.get('stream_offsets')
            for k, st in enumerate(s.streams):
                if st is not None:
                    out.append((('stream', i, k), offs[k], st.raw.size))
            for k, dr in enumerate(s.draws):
                out.append((('indices', i, k), dr.get('index_file_offset'), dr.get('index_size')))
        if self.shaders_raw:
            out.append((('shaders',), h.get('shaders_offset'), len(self.shaders_raw)))
        return out


def find_anchor(pieces, offset):
    """An offset -> (piece key, distance from its start), or None.

    A piece that starts at the offset wins over one that contains it.
    """
    for key, start, size in pieces:
        if offset == start:
            return (key, 0)
    for key, start, size in pieces:
        if start < offset < start + size:
            return (key, offset - start)
    return None
